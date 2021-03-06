string( REGEX REPLACE "/scala$" "/java" CMAKE_JAVA_DIR "${CMAKE_CURRENT_BINARY_DIR}" )
SET(CLASSPATH "${JBLAS}:${SHOGUN_JAR}:${Scala_JAR_EXECUTABLE}:${CMAKE_CURRENT_BINARY_DIR}")
SET(JAVA_LIB_PATH "${JAVA_MODULAR_BUILD_DIR}")

# add test case for each generated example
# (not generated yet so have to fake filenames from META_EXAMPLES list)
FOREACH(META_EXAMPLE ${META_EXAMPLES})
    # assume a structure <target_language>/<category>/listing.sg
	get_filename_component(EXAMPLE_NAME ${META_EXAMPLE} NAME_WE)
	IF(${CMAKE_VERSION} VERSION_LESS 2.8.12)
	    get_filename_component(FULL_DIR ${META_EXAMPLE} PATH)
	ELSE()
	    # only supported from >2.8.11
	    get_filename_component(FULL_DIR ${META_EXAMPLE} DIRECTORY)
	ENDIF()
	get_filename_component(EXAMPLE_REL_DIR ${FULL_DIR} NAME)
	set(EXAMPLE_NAME_WITH_DIR "${EXAMPLE_REL_DIR}-${EXAMPLE_NAME}")
	set(SCALA_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE_REL_DIR}")

	file(MAKE_DIRECTORY ${SCALA_FOLDER})

	add_test(NAME generated_scala-${EXAMPLE_NAME_WITH_DIR}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE_REL_DIR}
			COMMAND ${Scala_SCALA_EXECUTABLE}
				-cp ${CLASSPATH}:${CMAKE_JAVA_DIR}/${EXAMPLE_REL_DIR}
				-Djava.library.path=${JAVA_LIB_PATH} ${EXAMPLE_NAME})

ENDFOREACH()
