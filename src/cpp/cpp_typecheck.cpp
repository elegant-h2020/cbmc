/*******************************************************************\

Module: C++ Language Type Checking

Author: Daniel Kroening, kroening@cs.cmu.edu

\*******************************************************************/

/// \file
/// C++ Language Type Checking

#include "cpp_typecheck.h"

#include <util/pointer_expr.h>
#include <util/source_location.h>
#include <util/symbol_table.h>

#include <ansi-c/builtin_factory.h>

#include "cpp_declarator.h"
#include "cpp_util.h"
#include "expr2cpp.h"

void cpp_typecheckt::convert(cpp_itemt &item)
{
  if(item.is_declaration())
    convert(to_cpp_declaration(item));
  else if(item.is_linkage_spec())
    convert(item.get_linkage_spec());
  else if(item.is_namespace_spec())
    convert(item.get_namespace_spec());
  else if(item.is_using())
    convert(item.get_using());
  else if(item.is_static_assert())
    convert(item.get_static_assert());
  else
  {
    error().source_location=item.source_location();
    error() << "unknown parse-tree element: " << item.id() << eom;
    throw 0;
  }
}

/// typechecking main method
void cpp_typecheckt::typecheck()
{
  // default linkage is "automatic"
  current_linkage_spec=ID_auto;

  for(auto &item : cpp_parse_tree.items)
    convert(item);

  static_and_dynamic_initialization();

  typecheck_method_bodies();

  do_not_typechecked();

  clean_up();
}

const struct_typet &cpp_typecheckt::this_struct_type()
{
  const exprt &this_expr=
    cpp_scopes.current_scope().this_expr;

  CHECK_RETURN(this_expr.is_not_nil());
  CHECK_RETURN(this_expr.type().id() == ID_pointer);

  const typet &t = follow(to_pointer_type(this_expr.type()).base_type());
  CHECK_RETURN(t.id() == ID_struct);
  return to_struct_type(t);
}

std::string cpp_typecheckt::to_string(const exprt &expr)
{
  return expr2cpp(expr, *this);
}

std::string cpp_typecheckt::to_string(const typet &type)
{
  return type2cpp(type, *this);
}

bool cpp_typecheck(
  cpp_parse_treet &cpp_parse_tree,
  symbol_table_baset &symbol_table,
  const std::string &module,
  message_handlert &message_handler)
{
  cpp_typecheckt cpp_typecheck(
    cpp_parse_tree, symbol_table, module, message_handler);
  return cpp_typecheck.typecheck_main();
}

bool cpp_typecheck(
  exprt &expr,
  message_handlert &message_handler,
  const namespacet &ns)
{
  const unsigned errors_before=
    message_handler.get_message_count(messaget::M_ERROR);

  symbol_tablet symbol_table;
  cpp_parse_treet cpp_parse_tree;

  cpp_typecheckt cpp_typecheck(cpp_parse_tree, symbol_table,
                               ns.get_symbol_table(), "", message_handler);

  try
  {
    cpp_typecheck.typecheck_expr(expr);
  }

  catch(int)
  {
    cpp_typecheck.error();
  }

  catch(const char *e)
  {
    cpp_typecheck.error() << e << messaget::eom;
  }

  catch(const std::string &e)
  {
    cpp_typecheck.error() << e << messaget::eom;
  }

  catch(const invalid_source_file_exceptiont &e)
  {
    cpp_typecheck.error().source_location = e.get_source_location();
    cpp_typecheck.error() << e.get_reason() << messaget::eom;
  }

  return message_handler.get_message_count(messaget::M_ERROR)!=errors_before;
}

/// Initialization of static objects:
///
/// "Objects with static storage duration (3.7.1) shall be zero-initialized
/// (8.5) before any other initialization takes place. Zero-initialization
/// and initialization with a constant expression are collectively called
/// static initialization; all other initialization is dynamic
/// initialization. Objects of POD types (3.9) with static storage duration
/// initialized with constant expressions (5.19) shall be initialized before
/// any dynamic initialization takes place. Objects with static storage
/// duration defined in namespace scope in the same translation unit and
/// dynamically initialized shall be initialized in the order in which their
/// definition appears in the translation unit. [Note: 8.5.1 describes the
/// order in which aggregate members are initialized. The initialization
/// of local static objects is described in 6.7. ]"
void cpp_typecheckt::static_and_dynamic_initialization()
{
  code_blockt init_block; // Dynamic Initialization Block

  disable_access_control = true;

  for(const irep_idt &d_it : dynamic_initializations)
  {
    symbolt &symbol = symbol_table.get_writeable_ref(d_it);

    if(symbol.is_extern)
      continue;

    // PODs are always statically initialized
    if(cpp_is_pod(symbol.type))
      continue;

    DATA_INVARIANT(symbol.is_static_lifetime, "should be static");
    DATA_INVARIANT(!symbol.is_type, "should not be a type");
    DATA_INVARIANT(symbol.type.id() != ID_code, "should not be code");

    exprt symbol_expr=cpp_symbol_expr(symbol);

    // initializer given?
    if(symbol.value.is_not_nil())
    {
      // This will be a constructor call,
      // which we execute.
      init_block.add(to_code(symbol.value));

      // Make it nil to get zero initialization by
      // __CPROVER_initialize
      symbol.value.make_nil();
    }
    else
    {
      // use default constructor
      exprt::operandst ops;

      auto call = cpp_constructor(symbol.location, symbol_expr, ops);

      if(call.has_value())
        init_block.add(call.value());
    }
  }

  dynamic_initializations.clear();

  // Create the dynamic initialization procedure
  symbolt init_symbol{
    "#cpp_dynamic_initialization#" + id2string(module),
    code_typet({}, typet(ID_constructor)),
    ID_cpp};
  init_symbol.base_name="#cpp_dynamic_initialization#"+id2string(module);
  init_symbol.value.swap(init_block);
  init_symbol.module=module;

  symbol_table.insert(std::move(init_symbol));

  disable_access_control=false;
}

void cpp_typecheckt::do_not_typechecked()
{
  bool cont;

  do
  {
    cont = false;

    for(auto it = symbol_table.begin(); it != symbol_table.end(); ++it)
    {
      const symbolt &symbol = it->second;

      if(
        symbol.value.id() == ID_cpp_not_typechecked &&
        symbol.value.get_bool(ID_is_used))
      {
        DATA_INVARIANT(symbol.type.id() == ID_code, "must be code");
        exprt value = symbol.value;

        if(symbol.base_name=="operator=")
        {
          cpp_declaratort declarator;
          declarator.add_source_location() = symbol.location;
          default_assignop_value(
            lookup(symbol.type.get(ID_C_member_name)), declarator);
          value.swap(declarator.value());
          cont=true;
        }
        else if(symbol.value.operands().size()==1)
        {
          value = to_unary_expr(symbol.value).op();
          cont=true;
        }
        else
          UNREACHABLE; // Don't know what to do!

        symbolt &writable_symbol = it.get_writeable_symbol();
        writable_symbol.value.swap(value);
        convert_function(writable_symbol);
      }
    }
  }
  while(cont);

  for(auto it = symbol_table.begin(); it != symbol_table.end(); ++it)
  {
    if(it->second.value.id() == ID_cpp_not_typechecked)
      it.get_writeable_symbol().value.make_nil();
  }
}

void cpp_typecheckt::clean_up()
{
  auto it = symbol_table.begin();

  while(it != symbol_table.end())
  {
    auto cur_it = it;
    it++;

    const symbolt &symbol=cur_it->second;

    // erase templates and all member functions that have not been converted
    if(
      symbol.type.get_bool(ID_is_template) ||
      deferred_typechecking.find(symbol.name) != deferred_typechecking.end())
    {
      symbol_table.erase(cur_it);
      continue;
    }
    else if(symbol.type.id()==ID_struct ||
            symbol.type.id()==ID_union)
    {
      // remove methods from 'components'
      struct_union_typet &struct_union_type =
        to_struct_union_type(cur_it.get_writeable_symbol().type);

      const struct_union_typet::componentst &components=
        struct_union_type.components();

      struct_union_typet::componentst data_members;
      data_members.reserve(components.size());

      struct_union_typet::componentst &function_members=
        (struct_union_typet::componentst &)
        (struct_union_type.add(ID_methods).get_sub());

      function_members.reserve(components.size());

      for(const auto &compo_it : components)
      {
        if(compo_it.get_bool(ID_is_static) ||
           compo_it.get_bool(ID_is_type))
        {
          // skip it
        }
        else if(compo_it.type().id()==ID_code)
        {
          function_members.push_back(compo_it);
        }
        else
        {
          data_members.push_back(compo_it);
        }
      }

      struct_union_type.components().swap(data_members);
    }
  }
}

bool cpp_typecheckt::builtin_factory(const irep_idt &identifier)
{
  return ::builtin_factory(
    identifier, false, symbol_table, get_message_handler());
}

bool cpp_typecheckt::contains_cpp_name(const exprt &expr)
{
  if(expr.id() == ID_cpp_name || expr.id() == ID_cpp_declaration)
    return true;

  for(const exprt &op : expr.operands())
  {
    if(contains_cpp_name(op))
      return true;
  }
  return false;
}
