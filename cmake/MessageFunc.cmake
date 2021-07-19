# these functions are to print nice messages in some boxes ...

# you can control the output width with the DISPLAY_WIDTH var
# which as a default value set to set(DISPLAY_WIDTH 80)

# For instance

# messageTitle(" Example ")
# messageInfo(" Some information")
# messageSetting("DISPLAY_WIDTH")
# messageEnd()

# Outputs

# ┌────────────────────────────────┤│ Example │├──────────────────────────────────┐
# │  Some information                                                             │
# │ DISPLAY_WIDTH                  80                                             │
# └───────────────────────────────────────────────────────────────────────────────┘

# Or outputs

# ┌──┤│ Example │├────┐
# │  Some information │
# │ DISPLAY_WIDTH    ⏎│
# │                20 │
# └───────────────────┘

# Depending of the DISPLAY_WIDTH value

#set default value
if(NOT DEFINED DISPLAY_WIDTH)
    set(DISPLAY_WIDTH 80)
endif()

# messageFormat used internally
function(messageFormat TEXT PRE PRE_LEN POST POST_LEN FILL)
    set(TEXT_LEN 0)
    if(TEXT)
        string(LENGTH ${TEXT} TEXT_LEN)
    endif()
    MATH(EXPR LEN "${DISPLAY_WIDTH} - ${TEXT_LEN} - ${PRE_LEN} - ${POST_LEN}")
    set(FILL1 "")
    if(LEN LESS 0)
        MATH(EXPR SUBLEN "${DISPLAY_WIDTH} - ${PRE_LEN} - ${POST_LEN}-1")
        string(SUBSTRING ${TEXT} 0  ${SUBLEN} STRING1)
        MATH(EXPR PL2 "${POST_LEN}+1")
        messageFormat(${STRING1} ${PRE} ${PRE_LEN} "⏎${POST}" ${PL2} ${FILL})
        string(SUBSTRING ${TEXT} ${SUBLEN} -1 STRING2)
        messageFormat(${STRING2} ${PRE} ${PRE_LEN} ${POST} ${POST_LEN} ${FILL})
    else()
        foreach (i RANGE ${LEN})
            set(FILL1 "${FILL1}${FILL}")
        endforeach()
    
        message(NOTICE "${PRE}${TEXT}${FILL1}${POST}")
    endif()
endfunction()

function(messageInfo INFO)  
    set(PRE "│ ")
    set(PRE_LEN 2)
    set(POST "│")
    set(POST_LEN 1)
    messageFormat(${INFO} ${PRE} ${PRE_LEN} ${POST} ${POST_LEN} " ")
endfunction()

function(messageSetting OPTION)
    set(PRE "│ ")
    set(PRE_LEN 2)
    set(POST "│")
    set(POST_LEN 1)

    string(LENGTH ${OPTION} len)
    MATH(EXPR len "30-${len}")
    set(FILL "")
    foreach (i RANGE ${len})
        set(FILL "${FILL} ")
    endforeach()

    if(DEFINED ${OPTION})
        set(MSG "${OPTION}${FILL}${${OPTION}}")
    else()
        set(MSG "${OPTION}${FILL}[undef]")
    endif()
    messageFormat(${MSG} ${PRE} ${PRE_LEN} ${POST} ${POST_LEN} " ")
endfunction()

function(messageTitle TITLE)
    string(LENGTH ${TITLE} len)
    MATH(EXPR len "${DISPLAY_WIDTH}-6-${len}")
    MATH(EXPR len1 "${len}/2")
    MATH(EXPR len1 "${len1}-1")
    MATH(EXPR len2 "${len}-${len1}-1")
    set(FILL1 "")
    foreach (i RANGE ${len1})
        set(FILL1 "${FILL1}─")
    endforeach()
    set(FILL2 "")
    
    foreach (i RANGE ${len2})
        set(FILL2 "${FILL2}─")
    endforeach()
    message(NOTICE "┌${FILL1}┤│${TITLE}│├${FILL2}┐")
endfunction()

function(messageEnd)
    set(PRE "└")
    set(PRE_LEN 1)
    set(POST "┘")
    set(POST_LEN 1)
    messageFormat("" ${PRE} ${PRE_LEN} ${POST} ${POST_LEN} "─")
endfunction()
