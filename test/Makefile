all: ix_test

ix_test: ../src/ix_manager.cc ix_test.cc
	g++ ../src/ix_manager.cc ix_test.cc -o ix_test

red_ix_test: ../src/ix_manager.cc ../src/ix_test.cc
	g++ ../src/ix_manager.cc ../src/ix_test.cc -o red_ix_test