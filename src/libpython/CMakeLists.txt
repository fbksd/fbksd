
find_package(PythonLibs 3 REQUIRED)
find_package(Boost REQUIRED COMPONENTS python3 numpy3)

add_library(PyClient SHARED client.cpp __init__.py)
target_include_directories(PyClient PRIVATE SYSTEM ${PYTHON_INCLUDE_DIRS})
target_link_libraries(PyClient PUBLIC client PRIVATE Boost::python3 Boost::numpy3)
set_target_properties(PyClient PROPERTIES
    OUTPUT_NAME "client"
    PREFIX ""
)
install(TARGETS PyClient
    EXPORT fbksdTargets
    LIBRARY DESTINATION "./${FBKSD_PYTHON_PATH}/fbksd"
    ARCHIVE DESTINATION "./${FBKSD_PYTHON_PATH}/fbksd"
)

install(FILES "__init__.py" DESTINATION "./${FBKSD_PYTHON_PATH}/fbksd")
