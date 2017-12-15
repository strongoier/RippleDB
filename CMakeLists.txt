cmake_minimum_required(VERSION 3.9)
project(RippleDB)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "-m32 -O2")

set(GLOBAL_SOURCES src/global.h src/attr.cc src/rid.cc)

set(PF_SOURCES src/pf.h src/pf_internal.h src/pf_error.cc src/pf_buffermgr.cc src/pf_filehandle.cc src/pf_pagehandle.cc src/pf_hashtable.cc src/pf_manager.cc)

set(RM_SOURCES src/rm.h src/rm_error.cc src/rm_manager.cc src/rm_filehandle.cc src/rm_filescan.cc src/rm_record.cc)

set(IX_SOURCES src/ix.h src/ix_bplustree.cc src/ix_error.cc src/ix_indexhandle.cc src/ix_indexscan.cc src/ix_internal.cc src/ix_manager.cc)

add_executable(RippleDB src/ix_test.cc ${GLOBAL_SOURCES} ${PF_SOURCES} ${RM_SOURCES} ${IX_SOURCES})
