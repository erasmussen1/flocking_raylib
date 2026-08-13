
project(flocking_main)

add_executable(${PROJECT_NAME}
	main.cpp
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		raylib
        mather
        m
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources
     DESTINATION
        ${CMAKE_CURRENT_BINARY_DIR}
)

