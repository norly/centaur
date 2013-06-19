#!/bin/sh

BUILDDIR=${BUILDDIR:-build}
mkdir -p $BUILDDIR

function test_check_retval
{
  TEST_RETVAL=$?

  if [ $TEST_RETVAL != 0 ]
  then
    exit $TEST_RETVAL
  fi
}
