set(VENV_DIR "${CMAKE_SOURCE_DIR}/.venv")

if (NOT EXISTS "${VENV_DIR}/bin/activate")
    message(STATUS "Creating Python virtual environment at ${VENV_DIR}")
    execute_process(
            COMMAND python3 -m venv ${VENV_DIR}
            RESULT_VARIABLE VENV_RESULT
    )
    if (NOT VENV_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to create Python virtual environment.")
    endif ()

    set(PIP_EXECUTABLE "${VENV_DIR}/bin/pip")
    message(STATUS "Installing Python dependencies from requirements.txt")
    execute_process(
            COMMAND "${PIP_EXECUTABLE}" install --upgrade pip
            COMMAND "${PIP_EXECUTABLE}" install -r "${CMAKE_SOURCE_DIR}/requirements.txt"
            RESULT_VARIABLE PIP_RESULT
    )
    if (NOT PIP_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to install Python dependencies.")
    endif ()
endif ()
