// Author: Diffblue Ltd.

#include <util/arith_tools.h>
#include <util/bitvector_types.h>
#include <util/namespace.h>
#include <util/symbol_table.h>

#include <solvers/smt2_incremental/struct_encoding.h>
#include <testing-utils/use_catch.h>

struct struct_encoding_test_environmentt
{
  symbol_tablet symbol_table;
  namespacet ns{symbol_table};
  struct_encodingt struct_encoding{ns};

  static struct_encoding_test_environmentt make()
  {
    return {};
  }

private:
  struct_encoding_test_environmentt() = default;
};

TEST_CASE(
  "struct encoding of non-stuct type is a no-op",
  "[core][smt2_incremental]")
{
  auto test = struct_encoding_test_environmentt::make();
  typet input = signedbv_typet{8};
  REQUIRE(test.struct_encoding.encode(input) == input);
}

TEST_CASE("struct encoding of types", "[core][smt2_incremental]")
{
  const struct_union_typet::componentst components{
    {"foo", unsignedbv_typet{8}}, {"bar", signedbv_typet{16}}};
  struct_typet struct_type{components};
  type_symbolt type_symbol{"my_structt", struct_type, ID_C};
  auto test = struct_encoding_test_environmentt::make();
  test.symbol_table.insert(type_symbol);
  struct_tag_typet struct_tag{type_symbol.name};
  SECTION("direct struct_tag_type encoding")
  {
    REQUIRE(test.struct_encoding.encode(struct_tag) == bv_typet{24});
  }
  SECTION("array of structs encoding")
  {
    const auto index_type = signedbv_typet{32};
    const auto array_size = from_integer(5, index_type);
    array_typet array_of_struct{struct_tag, array_size};
    array_typet expected_encoded_array{bv_typet{24}, array_size};
    REQUIRE(
      test.struct_encoding.encode(array_of_struct) == expected_encoded_array);
  }
  SECTION("array of array of structs encoding")
  {
    const auto index_type = signedbv_typet{32};
    const auto array_size_inner = from_integer(4, index_type);
    const auto array_size_outer = from_integer(2, index_type);
    array_typet array_of_struct{struct_tag, array_size_inner};
    array_typet array_of_array_of_struct{array_of_struct, array_size_outer};
    array_typet expected_encoded_array{
      array_typet{bv_typet{24}, array_size_inner}, array_size_outer};
    REQUIRE(
      test.struct_encoding.encode(array_of_array_of_struct) ==
      expected_encoded_array);
  }
}
