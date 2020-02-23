add_custom_target(test)
add_custom_target(all_tests)

function(create_test name)
    add_executable(${name}_test ${name}.cpp)
    target_link_libraries(${name}_test gtest gtest_main core_lib)
    add_dependencies(test ${name}_test)

    add_custom_target(run_${name}_test COMMAND ${name}_test)
    add_dependencies(all_tests run_${name}_test)
endfunction()

create_test(Bits)
create_test(Card)
create_test(CardArray)
create_test(combinatorics)
create_test(Deal)
create_test(KnowableState)
create_test(random)
