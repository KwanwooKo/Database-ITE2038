add_test( BpTree.DescendingInsert /home/kwanwoo/Desktop/database/Project2/db_project/build/bin/db_test [==[--gtest_filter=BpTree.DescendingInsert]==] --gtest_also_run_disabled_tests)
set_tests_properties( BpTree.DescendingInsert PROPERTIES WORKING_DIRECTORY /home/kwanwoo/Desktop/database/Project2/db_project/build/test SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
add_test( BpTree.Scan /home/kwanwoo/Desktop/database/Project2/db_project/build/bin/db_test [==[--gtest_filter=BpTree.Scan]==] --gtest_also_run_disabled_tests)
set_tests_properties( BpTree.Scan PROPERTIES WORKING_DIRECTORY /home/kwanwoo/Desktop/database/Project2/db_project/build/test SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set( db_test_TESTS BpTree.DescendingInsert BpTree.Scan)