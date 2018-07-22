#--------------------------------------------------------
# The CMakeLists.txt for:                       uReflector
# Author(s):                              
#--------------------------------------------------------

SET(SRC
  Reflector.cpp
  Reflector_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(uReflector ${SRC})

TARGET_LINK_LIBRARIES(uReflector
   ${MOOS_LIBRARIES}
   mbutil
   m
   pthread)

