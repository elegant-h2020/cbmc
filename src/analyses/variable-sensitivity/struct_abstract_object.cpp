/*******************************************************************\

 Module: analyses variable-sensitivity

 Author: Thomas Kiley, thomas.kiley@diffblue.com

\*******************************************************************/

#include <util/std_types.h>
#include <util/std_expr.h>

#include "struct_abstract_object.h"


/*******************************************************************\

Function: struct_abstract_objectt::struct_abstract_objectt

  Inputs:
   type - the type the abstract_object is representing

 Outputs:

 Purpose:

\*******************************************************************/

struct_abstract_objectt::struct_abstract_objectt(const typet &t):
  abstract_objectt(t)
{
  assert(t.id()==ID_struct);
}

/*******************************************************************\

Function: struct_abstract_objectt::struct_abstract_objectt

  Inputs:
   type - the type the abstract_object is representing
   top - is the abstract_object starting as top
   bottom - is the abstract_object starting as bottom

 Outputs:

 Purpose: Start the abstract object at either top or bottom or neither
          Asserts if both top and bottom are true

\*******************************************************************/

struct_abstract_objectt::struct_abstract_objectt(
  const typet &t, bool tp, bool bttm):
    abstract_objectt(t, tp, bttm)
{
  assert(t.id()==ID_struct);
}

/*******************************************************************\

Function: struct_abstract_objectt::struct_abstract_objectt

  Inputs:
   old - the abstract object to copy from

 Outputs:

 Purpose:

\*******************************************************************/

struct_abstract_objectt::struct_abstract_objectt(
  const struct_abstract_objectt &old):
    abstract_objectt(old)
{}

/*******************************************************************\

Function: struct_abstract_objectt::struct_abstract_objectt

  Inputs:
   expr - the expression to use as the starting pointer for an abstract object

 Outputs:

 Purpose:

\*******************************************************************/

struct_abstract_objectt::struct_abstract_objectt(const constant_exprt &e):
  abstract_objectt(e)
{
  assert(e.type().id()==ID_struct);
}