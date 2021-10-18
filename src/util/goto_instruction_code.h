/*******************************************************************\

Module: Data structures representing instructions in a GOTO program

Author: Daniel Kroening, dkr@amazon.com

\*******************************************************************/

#ifndef CPROVER_UTIL_GOTO_INSTRUCTION_CODE_H
#define CPROVER_UTIL_GOTO_INSTRUCTION_CODE_H

#include "std_code.h"

/// A \ref codet representing an assignment in the program.
/// For example, if an expression `e1` is represented as an \ref exprt `expr1`
/// and an expression `e2` is represented as an \ref exprt `expr2`, the
/// assignment `e1 = e2;` can be represented as `code_assignt(expr1, expr2)`.
class code_assignt : public codet
{
public:
  code_assignt() : codet(ID_assign)
  {
    operands().resize(2);
  }

  code_assignt(exprt lhs, exprt rhs)
    : codet(ID_assign, {std::move(lhs), std::move(rhs)})
  {
  }

  code_assignt(exprt lhs, exprt rhs, source_locationt loc)
    : codet(ID_assign, {std::move(lhs), std::move(rhs)}, std::move(loc))
  {
  }

  exprt &lhs()
  {
    return op0();
  }

  exprt &rhs()
  {
    return op1();
  }

  const exprt &lhs() const
  {
    return op0();
  }

  const exprt &rhs() const
  {
    return op1();
  }

  static void check(
    const codet &code,
    const validation_modet vm = validation_modet::INVARIANT)
  {
    DATA_CHECK(
      vm, code.operands().size() == 2, "assignment must have two operands");
  }

  static void validate(
    const codet &code,
    const namespacet &,
    const validation_modet vm = validation_modet::INVARIANT)
  {
    check(code, vm);

    DATA_CHECK(
      vm,
      code.op0().type() == code.op1().type(),
      "lhs and rhs of assignment must have same type");
  }

  static void validate_full(
    const codet &code,
    const namespacet &ns,
    const validation_modet vm = validation_modet::INVARIANT)
  {
    for(const exprt &op : code.operands())
    {
      validate_full_expr(op, ns, vm);
    }

    validate(code, ns, vm);
  }

protected:
  using codet::op0;
  using codet::op1;
  using codet::op2;
  using codet::op3;
};

template <>
inline bool can_cast_expr<code_assignt>(const exprt &base)
{
  return detail::can_cast_code_impl(base, ID_assign);
}

inline void validate_expr(const code_assignt &x)
{
  code_assignt::check(x);
}

inline const code_assignt &to_code_assign(const codet &code)
{
  PRECONDITION(code.get_statement() == ID_assign);
  code_assignt::check(code);
  return static_cast<const code_assignt &>(code);
}

inline code_assignt &to_code_assign(codet &code)
{
  PRECONDITION(code.get_statement() == ID_assign);
  code_assignt::check(code);
  return static_cast<code_assignt &>(code);
}

/// A \ref codet representing the removal of a local variable going out of
/// scope.
class code_deadt : public codet
{
public:
  explicit code_deadt(symbol_exprt symbol) : codet(ID_dead, {std::move(symbol)})
  {
  }

  symbol_exprt &symbol()
  {
    return static_cast<symbol_exprt &>(op0());
  }

  const symbol_exprt &symbol() const
  {
    return static_cast<const symbol_exprt &>(op0());
  }

  const irep_idt &get_identifier() const
  {
    return symbol().get_identifier();
  }

  static void check(
    const codet &code,
    const validation_modet vm = validation_modet::INVARIANT)
  {
    DATA_CHECK(
      vm,
      code.operands().size() == 1,
      "removal (code_deadt) must have one operand");
    DATA_CHECK(
      vm,
      code.op0().id() == ID_symbol,
      "removing a non-symbol: " + id2string(code.op0().id()) + "from scope");
  }

protected:
  using codet::op0;
  using codet::op1;
  using codet::op2;
  using codet::op3;
};

template <>
inline bool can_cast_expr<code_deadt>(const exprt &base)
{
  return detail::can_cast_code_impl(base, ID_dead);
}

inline void validate_expr(const code_deadt &x)
{
  code_deadt::check(x);
}

inline const code_deadt &to_code_dead(const codet &code)
{
  PRECONDITION(code.get_statement() == ID_dead);
  code_deadt::check(code);
  return static_cast<const code_deadt &>(code);
}

inline code_deadt &to_code_dead(codet &code)
{
  PRECONDITION(code.get_statement() == ID_dead);
  code_deadt::check(code);
  return static_cast<code_deadt &>(code);
}

/// \ref codet representation of a function call statement.
/// The function call statement has three operands.
/// The first is the expression that is used to store the return value.
/// The second is the function called.
/// The third is a vector of argument values.
class code_function_callt : public codet
{
public:
  explicit code_function_callt(exprt _function)
    : codet(
        ID_function_call,
        {nil_exprt(), std::move(_function), exprt(ID_arguments)})
  {
  }

  typedef exprt::operandst argumentst;

  code_function_callt(exprt _lhs, exprt _function, argumentst _arguments)
    : codet(
        ID_function_call,
        {std::move(_lhs), std::move(_function), exprt(ID_arguments)})
  {
    arguments() = std::move(_arguments);
  }

  code_function_callt(exprt _function, argumentst _arguments)
    : code_function_callt(std::move(_function))
  {
    arguments() = std::move(_arguments);
  }

  exprt &lhs()
  {
    return op0();
  }

  const exprt &lhs() const
  {
    return op0();
  }

  exprt &function()
  {
    return op1();
  }

  const exprt &function() const
  {
    return op1();
  }

  argumentst &arguments()
  {
    return op2().operands();
  }

  const argumentst &arguments() const
  {
    return op2().operands();
  }

  static void check(
    const codet &code,
    const validation_modet vm = validation_modet::INVARIANT)
  {
    DATA_CHECK(
      vm,
      code.operands().size() == 3,
      "function calls must have three operands:\n1) expression to store the "
      "returned values\n2) the function being called\n3) the vector of "
      "arguments");
  }

  static void validate(
    const codet &code,
    const namespacet &,
    const validation_modet vm = validation_modet::INVARIANT)
  {
    check(code, vm);

    if(code.op0().id() != ID_nil)
      DATA_CHECK(
        vm,
        code.op0().type() == to_code_type(code.op1().type()).return_type(),
        "function returns expression of wrong type");
  }

  static void validate_full(
    const codet &code,
    const namespacet &ns,
    const validation_modet vm = validation_modet::INVARIANT)
  {
    for(const exprt &op : code.operands())
    {
      validate_full_expr(op, ns, vm);
    }

    validate(code, ns, vm);
  }

protected:
  using codet::op0;
  using codet::op1;
  using codet::op2;
  using codet::op3;
};

template <>
inline bool can_cast_expr<code_function_callt>(const exprt &base)
{
  return detail::can_cast_code_impl(base, ID_function_call);
}

inline void validate_expr(const code_function_callt &x)
{
  code_function_callt::check(x);
}

inline const code_function_callt &to_code_function_call(const codet &code)
{
  PRECONDITION(code.get_statement() == ID_function_call);
  code_function_callt::check(code);
  return static_cast<const code_function_callt &>(code);
}

inline code_function_callt &to_code_function_call(codet &code)
{
  PRECONDITION(code.get_statement() == ID_function_call);
  code_function_callt::check(code);
  return static_cast<code_function_callt &>(code);
}

/// An assumption, which must hold in subsequent code.
class code_assumet : public codet
{
public:
  explicit code_assumet(exprt expr) : codet(ID_assume, {std::move(expr)})
  {
  }

  const exprt &assumption() const
  {
    return op0();
  }

  exprt &assumption()
  {
    return op0();
  }

protected:
  using codet::op0;
  using codet::op1;
  using codet::op2;
  using codet::op3;
};

template <>
inline bool can_cast_expr<code_assumet>(const exprt &base)
{
  return detail::can_cast_code_impl(base, ID_assume);
}

inline void validate_expr(const code_assumet &x)
{
  validate_operands(x, 1, "assume must have one operand");
}

inline const code_assumet &to_code_assume(const codet &code)
{
  PRECONDITION(code.get_statement() == ID_assume);
  const code_assumet &ret = static_cast<const code_assumet &>(code);
  validate_expr(ret);
  return ret;
}

inline code_assumet &to_code_assume(codet &code)
{
  PRECONDITION(code.get_statement() == ID_assume);
  code_assumet &ret = static_cast<code_assumet &>(code);
  validate_expr(ret);
  return ret;
}

/// A non-fatal assertion, which checks a condition then permits execution to
/// continue.
class code_assertt : public codet
{
public:
  explicit code_assertt(exprt expr) : codet(ID_assert, {std::move(expr)})
  {
  }

  const exprt &assertion() const
  {
    return op0();
  }

  exprt &assertion()
  {
    return op0();
  }

protected:
  using codet::op0;
  using codet::op1;
  using codet::op2;
  using codet::op3;
};

template <>
inline bool can_cast_expr<code_assertt>(const exprt &base)
{
  return detail::can_cast_code_impl(base, ID_assert);
}

inline void validate_expr(const code_assertt &x)
{
  validate_operands(x, 1, "assert must have one operand");
}

inline const code_assertt &to_code_assert(const codet &code)
{
  PRECONDITION(code.get_statement() == ID_assert);
  const code_assertt &ret = static_cast<const code_assertt &>(code);
  validate_expr(ret);
  return ret;
}

inline code_assertt &to_code_assert(codet &code)
{
  PRECONDITION(code.get_statement() == ID_assert);
  code_assertt &ret = static_cast<code_assertt &>(code);
  validate_expr(ret);
  return ret;
}

/// A `codet` representing the declaration that an input of a particular
/// description has a value which corresponds to the value of a given expression
/// (or expressions).
/// When working with the C front end, calls to the `__CPROVER_input` intrinsic
/// can be added to the input code in order add instructions of this type to the
/// goto program.
/// The first argument is expected to be a C string denoting the input
/// identifier. The second argument is the expression for the input value.
class code_inputt : public codet
{
public:
  /// This constructor is for support of calls to `__CPROVER_input` in user
  /// code. Where the first first argument is a description which may be any
  /// `const char *` and one or more corresponding expression arguments follow.
  explicit code_inputt(
    std::vector<exprt> arguments,
    optionalt<source_locationt> location = {});

  /// This constructor is intended for generating input instructions as part of
  /// synthetic entry point code, rather than as part of user code.
  /// \param description: This is used to construct an expression for a pointer
  ///   to a string constant containing the description text. This expression
  ///   is then used as the first argument.
  /// \param expression: This expression corresponds to a value which should be
  ///   recorded as an input.
  /// \param location: A location to associate with this instruction.
  code_inputt(
    const irep_idt &description,
    exprt expression,
    optionalt<source_locationt> location = {});

  static void check(
    const codet &code,
    const validation_modet vm = validation_modet::INVARIANT);
};

template <>
inline bool can_cast_expr<code_inputt>(const exprt &base)
{
  return detail::can_cast_code_impl(base, ID_input);
}

inline void validate_expr(const code_inputt &input)
{
  code_inputt::check(input);
}

/// A `codet` representing the declaration that an output of a particular
/// description has a value which corresponds to the value of a given expression
/// (or expressions).
/// When working with the C front end, calls to the `__CPROVER_output` intrinsic
/// can be added to the input code in order add instructions of this type to the
/// goto program.
/// The first argument is expected to be a C string denoting the output
/// identifier. The second argument is the expression for the output value.
class code_outputt : public codet
{
public:
  /// This constructor is for support of calls to `__CPROVER_output` in user
  /// code. Where the first first argument is a description which may be any
  /// `const char *` and one or more corresponding expression arguments follow.
  explicit code_outputt(
    std::vector<exprt> arguments,
    optionalt<source_locationt> location = {});

  /// This constructor is intended for generating output instructions as part of
  /// synthetic entry point code, rather than as part of user code.
  /// \param description: This is used to construct an expression for a pointer
  ///   to a string constant containing the description text.
  /// \param expression: This expression corresponds to a value which should be
  ///   recorded as an output.
  /// \param location: A location to associate with this instruction.
  code_outputt(
    const irep_idt &description,
    exprt expression,
    optionalt<source_locationt> location = {});

  static void check(
    const codet &code,
    const validation_modet vm = validation_modet::INVARIANT);
};

template <>
inline bool can_cast_expr<code_outputt>(const exprt &base)
{
  return detail::can_cast_code_impl(base, ID_output);
}

inline void validate_expr(const code_outputt &output)
{
  code_outputt::check(output);
}

#endif // CPROVER_GOTO_PROGRAMS_GOTO_INSTRUCTION_CODE_H