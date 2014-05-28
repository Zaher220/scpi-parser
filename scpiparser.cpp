#include "scpiparser.h"

SCPIParser::SCPIParser(QObject *parent) :
    QObject(parent)
{
}




/**
 * Find command termination character
 * @param cmd - input command
 * @param len - max search length
 * @return position of terminator or len
 */
size_t SCPIParser::cmdTerminatorPos(const char * cmd, size_t len) {
    const char * terminator = strnpbrk(cmd, len, "; \r\n\t");
    if (terminator == NULL) {
        return len;
    } else {
        return terminator - cmd;
    }
}

/**
 * Find command line separator
 * @param cmd - input command
 * @param len - max search length
 * @return pointer to line separator or NULL
 */
const char * SCPIParser::cmdlineSeparator(const char * cmd, size_t len) {
    return strnpbrk(cmd, len, ";\r\n");
}

/**
 * Find command line terminator
 * @param cmd - input command
 * @param len - max search length
 * @return pointer to command line terminator or NULL
 */
const char * SCPIParser::cmdlineTerminator(const char * cmd, size_t len) {
    return strnpbrk(cmd, len, "\r\n");
}

/**
 * Find command line separator position
 * @param cmd - input command
 * @param len - max search length
 * @return position of line separator or len
 */
size_t SCPIParser::cmdlineSeparatorPos(const char * cmd, size_t len) {
    const char * separator = cmdlineSeparator(cmd, len);
    if (separator == NULL) {
        return len;
    } else {
        return separator - cmd;
    }
}

/**
 * Find next part of command
 * @param cmd - input command
 * @param len - max search length
 * @return number of characters to be skipped
 */
size_t SCPIParser::skipCmdLine(const char * cmd, size_t len) {
    const char * separator = cmdlineSeparator(cmd, len);
    if (separator == NULL) {
        return len;
    } else {
        return separator + 1 - cmd;
    }
}

/**
 * Write data to SCPI output
 * @param context
 * @param data
 * @param len - lenght of data to be written
 * @return number of bytes written
 */
size_t SCPIParser::writeData(scpi_t * context, const char * data, size_t len) {
    return context->interface->write(context, data, len);
}

/**
 * Flush data to SCPI output
 * @param context
 * @return
 */
int SCPIParser::flushData(scpi_t * context) {
    if (context && context->interface && context->interface->flush) {
        return context->interface->flush(context);
    } else {
        return SCPI_RES_OK;
    }
}

/**
 * Write result delimiter to output
 * @param context
 * @return number of bytes written
 */
size_t SCPIParser::writeDelimiter(scpi_t * context) {
    if (context->output_count > 0) {
        return writeData(context, ", ", 2);
    } else {
        return 0;
    }
}

/**
 * Conditionaly write "New Line"
 * @param context
 * @return number of characters written
 */
size_t SCPIParser::writeNewLine(scpi_t * context) {
    if (context->output_count > 0) {
        size_t len;
        len = writeData(context, "\r\n", 2);
        flushData(context);
        return len;
    } else {
        return 0;
    }
}

/**
 * Process command
 * @param context
 */
void SCPIParser::processCommand(scpi_t * context) {
    const scpi_command_t * cmd = context->paramlist.cmd;

    context->cmd_error = FALSE;
    context->output_count = 0;
    context->input_count = 0;

    SCPI_DEBUG_COMMAND(context);
    /* if callback exists - call command callback */
    if (cmd->callback != NULL) {
        if ((cmd->callback(context) != SCPI_RES_OK) && !context->cmd_error) {
            SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        }
    }

    /* conditionaly write new line */
    writeNewLine(context);

    /* skip all whitespaces */
    paramSkipWhitespace(context);

    /* set error if command callback did not read all parameters */
    if (context->paramlist.length != 0 && !context->cmd_error) {
        SCPI_ErrorPush(context, SCPI_ERROR_PARAMETER_NOT_ALLOWED);
    }
}

/**
 * Cycle all patterns and search matching pattern. Execute command callback.
 * @param context
 * @result TRUE if context->paramlist is filled with correct values
 */
scpi_bool_t SCPIParser::findCommand(scpi_t * context, const char * cmdline_ptr, size_t cmdline_len, size_t cmd_len) {
    int32_t i;
    const scpi_command_t * cmd;

    for (i = 0; context->cmdlist[i].pattern != NULL; i++) {
        cmd = &context->cmdlist[i];
        if (matchCommand(cmd->pattern, cmdline_ptr, cmd_len)) {
            context->paramlist.cmd = cmd;
            context->paramlist.parameters = cmdline_ptr + cmd_len;
            context->paramlist.length = cmdline_len - cmd_len;
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * Parse one command line
 * @param context
 * @param data - complete command line
 * @param len - command line length
 * @return 1 if the last evaluated command was found
 */
int SCPIParser::SCPI_Parse(scpi_t * context, char * data, size_t len) {
    int result = 0;
    const char * cmdline_end = data + len;
    char * cmdline_ptr = data;
    size_t cmd_len;
    size_t cmdline_len;
    char * cmdline_ptr_prev = NULL;
    size_t cmd_len_prev = 0;

    if (context == NULL) {
        return -1;
    }

    while (cmdline_ptr < cmdline_end) {
        result = 0;
        cmd_len = cmdTerminatorPos(cmdline_ptr, cmdline_end - cmdline_ptr);
        if (cmd_len > 0) {
            composeCompoundCommand(cmdline_ptr_prev, cmd_len_prev,
                                   &cmdline_ptr, &cmd_len);
            cmdline_len = cmdlineSeparatorPos(cmdline_ptr, cmdline_end - cmdline_ptr);
            if(findCommand(context, cmdline_ptr, cmdline_len, cmd_len)) {
                processCommand(context);
                result = 1;
                cmdline_ptr_prev = cmdline_ptr;
                cmd_len_prev = cmd_len;
            } else {
                SCPI_ErrorPush(context, SCPI_ERROR_UNDEFINED_HEADER);
            }
        }
        cmdline_ptr += skipCmdLine(cmdline_ptr, cmdline_end - cmdline_ptr);
        cmdline_ptr += skipWhitespace(cmdline_ptr, cmdline_end - cmdline_ptr);
    }
    return result;
}

/**
 * Initialize SCPI context structure
 * @param context
 * @param command_list
 * @param buffer
 * @param interface
 */
void SCPIParser::SCPI_Init(scpi_t * context) {
    if (context->idn[0] == NULL) {
        context->idn[0] = SCPI_DEFAULT_1_MANUFACTURE;
    }
    if (context->idn[1] == NULL) {
        context->idn[1] = SCPI_DEFAULT_2_MODEL;
    }
    if (context->idn[2] == NULL) {
        context->idn[2] = SCPI_DEFAULT_3;
    }
    if (context->idn[3] == NULL) {
        context->idn[3] = SCPI_DEFAULT_4_REVISION;
    }

    context->buffer.position = 0;
    SCPI_ErrorInit(context);
}

/**
 * Interface to the application. Adds data to system buffer and try to search
 * command line termination. If the termination is found or if len=0, command
 * parser is called.
 *
 * @param context
 * @param data - data to process
 * @param len - length of data
 * @return
 */
int SCPIParser::SCPI_Input(scpi_t * context, const char * data, size_t len) {
    int result = 0;
    const char * cmd_term;
    if (len == 0) {
        context->buffer.data[context->buffer.position] = 0;
        result = SCPI_Parse(context, context->buffer.data, context->buffer.position);
        context->buffer.position = 0;
    } else {
        size_t buffer_free;
        int ws;
        buffer_free = context->buffer.length - context->buffer.position;
        if (len > (buffer_free - 1)) {
            return -1;
        }
        memcpy(&context->buffer.data[context->buffer.position], data, len);
        context->buffer.position += len;
        context->buffer.data[context->buffer.position] = 0;

        ws = skipWhitespace(context->buffer.data, context->buffer.position);
        cmd_term = cmdlineTerminator(context->buffer.data + ws, context->buffer.position - ws);
        while (cmd_term != NULL) {
            int curr_len = cmd_term - context->buffer.data;
            result = SCPI_Parse(context, context->buffer.data + ws, curr_len - ws);
            memmove(context->buffer.data, cmd_term, context->buffer.position - curr_len);
            context->buffer.position -= curr_len;

            ws = skipWhitespace(context->buffer.data, context->buffer.position);
            cmd_term = cmdlineTerminator(context->buffer.data + ws, context->buffer.position - ws);
        }
    }

    return result;
}

/* writing results */

/**
 * Write raw string result to the output
 * @param context
 * @param data
 * @return
 */
size_t SCPIParser::SCPI_ResultString(scpi_t * context, const char * data) {
    size_t len = strlen(data);
    size_t result = 0;
    result += writeDelimiter(context);
    result += writeData(context, data, len);
    context->output_count++;
    return result;
}

/**
 * Write integer value to the result
 * @param context
 * @param val
 * @return
 */
size_t SCPIParser::SCPI_ResultInt(scpi_t * context, int32_t val) {
    char buffer[12];
    size_t result = 0;
    size_t len = longToStr(val, buffer, sizeof (buffer));
    result += writeDelimiter(context);
    result += writeData(context, buffer, len);
    context->output_count++;
    return result;
}

/**
 * Write boolean value to the result
 * @param context
 * @param val
 * @return
 */
size_t SCPIParser::SCPI_ResultBool(scpi_t * context, scpi_bool_t val) {
    return SCPI_ResultInt(context, val ? 1 : 0);
}

/**
 * Write double walue to the result
 * @param context
 * @param val
 * @return
 */
size_t SCPIParser::SCPI_ResultDouble(scpi_t * context, double val) {
    char buffer[32];
    size_t result = 0;
    size_t len = doubleToStr(val, buffer, sizeof (buffer));
    result += writeDelimiter(context);
    result += writeData(context, buffer, len);
    context->output_count++;
    return result;

}

/**
 * Write string withn " to the result
 * @param context
 * @param data
 * @return
 */
size_t SCPIParser::SCPI_ResultText(scpi_t * context, const char * data) {
    size_t result = 0;
    result += writeDelimiter(context);
    result += writeData(context, "\"", 1);
    result += writeData(context, data, strlen(data));
    result += writeData(context, "\"", 1);
    context->output_count++;
    return result;
}

/* parsing parameters */

/**
 * Skip num bytes from the begginig of parameters
 * @param context
 * @param num
 */
void SCPIParser::paramSkipBytes(scpi_t * context, size_t num) {
    if (context->paramlist.length < num) {
        num = context->paramlist.length;
    }
    context->paramlist.parameters += num;
    context->paramlist.length -= num;
}

/**
 * Skip white spaces from the beggining of parameters
 * @param context
 */
void SCPIParser::paramSkipWhitespace(scpi_t * context) {
    size_t ws = skipWhitespace(context->paramlist.parameters, context->paramlist.length);
    paramSkipBytes(context, ws);
}

/**
 * Find next parameter
 * @param context
 * @param mandatory
 * @return
 */
scpi_bool_t SCPIParser::paramNext(scpi_t * context, scpi_bool_t mandatory) {
    paramSkipWhitespace(context);
    if (context->paramlist.length == 0) {
        if (mandatory) {
            SCPI_ErrorPush(context, SCPI_ERROR_MISSING_PARAMETER);
        }
        return FALSE;
    }
    if (context->input_count != 0) {
        if (context->paramlist.parameters[0] == ',') {
            paramSkipBytes(context, 1);
            paramSkipWhitespace(context);
        } else {
            SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SEPARATOR);
            return FALSE;
        }
    }
    context->input_count++;
    return TRUE;
}

/**
 * Parse integer parameter
 * @param context
 * @param value
 * @param mandatory
 * @return
 */
scpi_bool_t SCPIParser::SCPI_ParamInt(scpi_t * context, int32_t * value, scpi_bool_t mandatory) {
    const char * param;
    size_t param_len;
    size_t num_len;

    if (!value) {
        return FALSE;
    }

    if (!SCPI_ParamString(context, &param, &param_len, mandatory)) {
        return FALSE;
    }

    num_len = strToLong(param, value);

    if (num_len != param_len) {
        SCPI_ErrorPush(context, SCPI_ERROR_SUFFIX_NOT_ALLOWED);
        return FALSE;
    }

    return TRUE;
}

/**
 * Parse double parameter
 * @param context
 * @param value
 * @param mandatory
 * @return
 */
scpi_bool_t SCPIParser::SCPI_ParamDouble(scpi_t * context, double * value, scpi_bool_t mandatory) {
    const char * param;
    size_t param_len;
    size_t num_len;

    if (!value) {
        return FALSE;
    }

    if (!SCPI_ParamString(context, &param, &param_len, mandatory)) {
        return FALSE;
    }

    num_len = strToDouble(param, value);

    if (num_len != param_len) {
        SCPI_ErrorPush(context, SCPI_ERROR_SUFFIX_NOT_ALLOWED);
        return FALSE;
    }

    return TRUE;
}

/**
 * Parse string parameter
 * @param context
 * @param value
 * @param len
 * @param mandatory
 * @return
 */
scpi_bool_t SCPIParser::SCPI_ParamString(scpi_t * context, const char ** value, size_t * len, scpi_bool_t mandatory) {
    size_t length;

    if (!value || !len) {
        return FALSE;
    }

    if (!paramNext(context, mandatory)) {
        return FALSE;
    }

    if (locateStr(context->paramlist.parameters, context->paramlist.length, value, &length)) {
        paramSkipBytes(context, length);
        paramSkipWhitespace(context);
        if (len) {
            *len = length;
        }
        return TRUE;
    }

    return FALSE;
}

/**
 * Parse text parameter (can be inside "")
 * @param context
 * @param value
 * @param len
 * @param mandatory
 * @return
 */
scpi_bool_t SCPIParser::SCPI_ParamText(scpi_t * context, const char ** value, size_t * len, scpi_bool_t mandatory) {
    size_t length;

    if (!value || !len) {
        return FALSE;
    }

    if (!paramNext(context, mandatory)) {
        return FALSE;
    }

    if (locateText(context->paramlist.parameters, context->paramlist.length, value, &length)) {
        paramSkipBytes(context, length);
        if (len) {
            *len = length;
        }
        return TRUE;
    }

    return FALSE;
}

/**
 * Parse boolean parameter as described in the spec SCPI-99 7.3 Boolean Program Data
 * @param context
 * @param value
 * @param mandatory
 * @return
 */
scpi_bool_t SCPIParser::SCPI_ParamBool(scpi_t * context, scpi_bool_t * value, scpi_bool_t mandatory) {
    const char * param;
    size_t param_len;
    size_t num_len;
    int32_t i;

    if (!value) {
        return FALSE;
    }

    if (!SCPI_ParamString(context, &param, &param_len, mandatory)) {
        return FALSE;
    }

    if (matchPattern("ON", 2, param, param_len)) {
        *value = TRUE;
    } else if (matchPattern("OFF", 3, param, param_len)) {
        *value = FALSE;
    } else {
        num_len = strToLong(param, &i);

        if (num_len != param_len) {
            SCPI_ErrorPush(context, SCPI_ERROR_SUFFIX_NOT_ALLOWED);
            return FALSE;
        }

        *value = i ? TRUE : FALSE;
    }

    return TRUE;
}

/**
 * Parse choice parameter
 * @param context
 * @param options
 * @param value
 * @param mandatory
 * @return
 */
scpi_bool_t SCPIParser::SCPI_ParamChoice(scpi_t * context, const char * options[], int32_t * value, scpi_bool_t mandatory) {
    const char * param;
    size_t param_len;
    size_t res;

    if (!options || !value) {
        return FALSE;
    }

    if (!SCPI_ParamString(context, &param, &param_len, mandatory)) {
        return FALSE;
    }

    for (res = 0; options[res]; ++res) {
        if (matchPattern(options[res], strlen(options[res]), param, param_len)) {
            *value = res;
            return TRUE;
        }
    }

    SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
    return FALSE;
}














void SCPIParser::SCPI_ErrorInit(scpi_t *context)
{
    /*
     * // FreeRTOS
     * context->error_queue = (scpi_error_queue_t)xQueueCreate(100, sizeof(int16_t));
     */

    /* basic FIFO */
    context->error_queue = (scpi_error_queue_t)&local_error_queue;
    fifo_init((fifo_t *)context->error_queue);
}

/**
 * Clear error queue
 * @param context - scpi context
 */
void SCPIParser::SCPI_ErrorClear(scpi_t *context)
{
    /*
     * // FreeRTOS
     * xQueueReset((xQueueHandle)context->error_queue);
     */

    /* basic FIFO */
    fifo_clear((fifo_t *)context->error_queue);
}

/**
 * Pop error from queue
 * @param context - scpi context
 * @return error number
 */
int16_t SCPIParser::SCPI_ErrorPop(scpi_t *context)
{
    int16_t result = 0;

    /*
     * // FreeRTOS
     * if (pdFALSE == xQueueReceive((xQueueHandle)context->error_queue, &result, 0)) {
     *   result = 0;
     * }
     */

    /* basic FIFO */
    fifo_remove((fifo_t *)context->error_queue, &result);

    return result;
}

/**
 * Push error to queue
 * @param context - scpi context
 * @param err - error number
 */
void SCPIParser::SCPI_ErrorPush(scpi_t * context, int16_t err) {

    int i;

    SCPI_ErrorAddInternal(context, err);

    for(i = 0; i < ERROR_DEFS_N; i++) {
        if ((err <= errs[i].from) && (err >= errs[i].to)) {
            SCPI_RegSetBits(context, SCPI_REG_ESR, errs[i].bit);
        }
    }

    if (context) {
        if (context->interface && context->interface->error) {
            context->interface->error(context, err);
        }

        context->cmd_error = TRUE;
    }
}

/**
 * Return number of errors/events in the queue
 * @param context
 * @return
 */
int32_t SCPIParser::SCPI_ErrorCount(scpi_t *context)
{
    int16_t result = 0;

    /*
     * // FreeRTOS
     * result = uxQueueMessagesWaiting((xQueueHandle)context->error_queue);
     */

    /* basic FIFO */
    fifo_count((fifo_t *)context->error_queue, &result);

    return result;
}

/**
 * Translate error number to string
 * @param err - error number
 * @return Error string representation
 */
const char *SCPIParser::SCPI_ErrorTranslate(int16_t err)
{
    switch (err) {
    case 0: return "No error";
#define X(def, val, str) case def: return str;
        LIST_OF_ERRORS
        #undef X
            default: return "Unknown error";
    }
}

void SCPIParser::SCPI_ErrorAddInternal(scpi_t * context, int16_t err) {
    /*
     * // FreeRTOS
     * xQueueSend((xQueueHandle)context->error_queue, &err, 0);
     */

    /* basic FIFO */
    fifo_add((fifo_t *)context->error_queue, err);
}




/**
 * Debug function: show current command and its parameters
 * @param context
 * @return
 */
scpi_bool_t SCPIParser::SCPI_DebugCommand(scpi_t * context) {
    size_t res;
    printf("**DEBUG: %s (\"", context->paramlist.cmd->pattern);
    res = fwrite(context->paramlist.parameters, 1, context->paramlist.length, stdout);
    (void)res;
    printf("\" - %lu\r\n", (unsigned long)context->paramlist.length);

    return TRUE;
}


void SCPIParser::fifo_init(fifo_t * fifo) {
    fifo->wr = 0;
    fifo->rd = 0;
    fifo->size = FIFO_SIZE;
}

void SCPIParser::fifo_clear(fifo_t * fifo) {
    fifo->wr = 0;
    fifo->rd = 0;
}

scpi_bool_t SCPIParser::fifo_add(fifo_t * fifo, int16_t value) {
    /* FIFO full? */
    if (fifo->wr == ((fifo->rd + fifo->size) % (fifo->size + 1))) {
        fifo_remove(fifo, NULL);
    }

    fifo->data[fifo->wr] = value;
    fifo->wr = (fifo->wr + 1) % (fifo->size + 1);

    return TRUE;
}

scpi_bool_t SCPIParser::fifo_remove(fifo_t * fifo, int16_t * value) {
    /* FIFO empty? */
    if (fifo->wr == fifo->rd) {
        return FALSE;
    }

    if(value) {
        *value = fifo->data[fifo->rd];
    }

    fifo->rd = (fifo->rd + 1) % (fifo->size + 1);

    return TRUE;
}

scpi_bool_t SCPIParser::fifo_count(fifo_t * fifo, int16_t * value) {
    *value = fifo->wr - fifo->rd;
    if (*value < 0) {
        *value += (fifo->size + 1);
    }
    return TRUE;
}



/**
 * Match string constant to one of special number values
 * @param specs specifications of special numbers (patterns)
 * @param str string to be recognised
 * @param len length of string
 * @param value resultin value
 * @return TRUE if str matches one of specs patterns
 */
scpi_bool_t SCPIParser::translateSpecialNumber(const scpi_special_number_def_t * specs, const char * str, size_t len, scpi_number_t * value) {
    int i;

    value->value = 0.0;
    value->unit = SCPI_UNIT_NONE;
    value->type = SCPI_NUM_NUMBER;

    if (specs == NULL) {
        return FALSE;
    }

    for (i = 0; specs[i].name != NULL; i++) {
        if (matchPattern(specs[i].name, strlen(specs[i].name), str, len)) {
            value->type = specs[i].type;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Convert special number type to its string representation
 * @param specs specifications of special numbers (patterns)
 * @param type type of special number
 * @return String representing special number or NULL
 */
const char * SCPIParser::translateSpecialNumberInverse(const scpi_special_number_def_t * specs, scpi_special_number_t type) {
    int i;

    if (specs == NULL) {
        return NULL;
    }

    for (i = 0; specs[i].name != NULL; i++) {
        if (specs[i].type == type) {
            return specs[i].name;
        }
    }

    return NULL;
}

/**
 * Convert string describing unit to its representation
 * @param units units patterns
 * @param unit text representation of unknown unit
 * @param len length of text representation
 * @return pointer of related unit definition or NULL
 */
const scpi_unit_def_t * SCPIParser::translateUnit(const scpi_unit_def_t * units, const char * unit, size_t len) {
    int i;

    if (units == NULL) {
        return NULL;
    }

    for (i = 0; units[i].name != NULL; i++) {
        if (compareStr(unit, len, units[i].name, strlen(units[i].name))) {
            return &units[i];
        }
    }

    return NULL;
}

/**
 * Convert unit definition to string
 * @param units units definitions (patterns)
 * @param unit type of unit
 * @return string representation of unit
 */
const char * SCPIParser::translateUnitInverse(const scpi_unit_def_t * units, const scpi_unit_t unit) {
    int i;

    if (units == NULL) {
        return NULL;
    }

    for (i = 0; units[i].name != NULL; i++) {
        if ((units[i].unit == unit) && (units[i].mult == 1)) {
            return units[i].name;
        }
    }

    return NULL;
}

/**
 * Transform number to base units
 * @param context
 * @param unit text representation of unit
 * @param len length of text representation
 * @param value preparsed numeric value
 * @return TRUE if value parameter was converted to base units
 */
scpi_bool_t SCPIParser::transformNumber(scpi_t * context, const char * unit, size_t len, scpi_number_t * value) {
    size_t s;
    const scpi_unit_def_t * unitDef;
    s = skipWhitespace(unit, len);

    if (s == len) {
        value->unit = SCPI_UNIT_NONE;
        return TRUE;
    }

    unitDef = translateUnit(context->units, unit + s, len - s);

    if (unitDef == NULL) {
        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SUFFIX);
        return FALSE;
    }

    value->value *= unitDef->mult;
    value->unit = unitDef->unit;

    return TRUE;
}

/**
 * Parse parameter as number, number with unit or special value (min, max, default, ...)
 * @param context
 * @param value return value
 * @param mandatory if the parameter is mandatory
 * @return
 */
scpi_bool_t SCPIParser::SCPI_ParamNumber(scpi_t * context, scpi_number_t * value, scpi_bool_t mandatory) {
    scpi_bool_t result;
    const char * param;
    size_t len;
    size_t numlen;

    /* read parameter and shift to the next one */
    result = SCPI_ParamString(context, &param, &len, mandatory);

    /* value not initializes */
    if (!value) {
        return FALSE;
    }

    value->type = SCPI_NUM_DEF;

    /* if parameter was not found, return TRUE or FALSE according
     * to fact that parameter was mandatory or not */
    if (!result) {
        return mandatory ? FALSE : TRUE;
    }

    /* convert string to special number type */
    if (translateSpecialNumber(context->special_numbers, param, len, value)) {
        /* found special type */
        return TRUE;
    }

    /* convert text from double - no special type */
    numlen = strToDouble(param, &value->value);

    /* transform units of value */
    if (numlen <= len) {
        return transformNumber(context, param + numlen, len - numlen, value);
    }
    return FALSE;

}

/**
 * Convert scpi_number_t to string
 * @param context
 * @param value number value
 * @param str target string
 * @param len max length of string
 * @return number of chars written to string
 */
size_t SCPIParser::SCPI_NumberToStr(scpi_t * context, scpi_number_t * value, char * str, size_t len) {
    const char * type;
    const char * unit;
    size_t result;

    if (!value || !str) {
        return 0;
    }

    type = translateSpecialNumberInverse(context->special_numbers, value->type);

    if (type) {
        strncpy(str, type, len);
        return min(strlen(type), len);
    }

    result = doubleToStr(value->value, str, len);

    unit = translateUnitInverse(context->units, value->unit);

    if (unit) {
        strncat(str, " ", len);
        strncat(str, unit, len);
        result += strlen(unit) + 1;
    }

    return result;
}


/**
 * Update register value
 * @param context
 * @param name - register name
 */
void SCPIParser::regUpdate(scpi_t * context, scpi_reg_name_t name) {
    SCPI_RegSet(context, name, SCPI_RegGet(context, name));
}

/**
 * Update STB register according to value and its mask register
 * @param context
 * @param val value of register
 * @param mask name of mask register (enable register)
 * @param stbBits bits to clear or set in STB
 */
void SCPIParser::regUpdateSTB(scpi_t * context, scpi_reg_val_t val, scpi_reg_name_t mask, scpi_reg_val_t stbBits) {
    if (val & SCPI_RegGet(context, mask)) {
        SCPI_RegSetBits(context, SCPI_REG_STB, stbBits);
    } else {
        SCPI_RegClearBits(context, SCPI_REG_STB, stbBits);
    }
}

/**
 * Get register value
 * @param name - register name
 * @return register value
 */
scpi_reg_val_t SCPIParser::SCPI_RegGet(scpi_t * context, scpi_reg_name_t name) {
    if ((name < SCPI_REG_COUNT) && (context->registers != NULL)) {
        return context->registers[name];
    } else {
        return 0;
    }
}

/**
 * Wrapper function to control interface from context
 * @param context
 * @param ctrl number of controll message
 * @param value value of related register
 */
size_t SCPIParser::writeControl(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {
    if (context && context->interface && context->interface->control) {
        return context->interface->control(context, ctrl, val);
    } else {
        return 0;
    }
}

/**
 * Set register value
 * @param name - register name
 * @param val - new value
 */
void SCPIParser::SCPI_RegSet(scpi_t * context, scpi_reg_name_t name, scpi_reg_val_t val) {
    scpi_bool_t srq = FALSE;
    scpi_reg_val_t mask;
    scpi_reg_val_t old_val;

    if ((name >= SCPI_REG_COUNT) || (context->registers == NULL)) {
        return;
    }

    /* store old register value */
    old_val = context->registers[name];

    /* set register value */
    context->registers[name] = val;

    /** @TODO: remove recutsion */
    switch (name) {
    case SCPI_REG_STB:
        mask = SCPI_RegGet(context, SCPI_REG_SRE);
        mask &= ~STB_SRQ;
        if (val & mask) {
            val |= STB_SRQ;
            /* avoid sending SRQ if nothing has changed */
            if (old_val != val) {
                srq = TRUE;
            }
        } else {
            val &= ~STB_SRQ;
        }
        break;
    case SCPI_REG_SRE:
        regUpdate(context, SCPI_REG_STB);
        break;
    case SCPI_REG_ESR:
        regUpdateSTB(context, val, SCPI_REG_ESE, STB_ESR);
        break;
    case SCPI_REG_ESE:
        regUpdate(context, SCPI_REG_ESR);
        break;
    case SCPI_REG_QUES:
        regUpdateSTB(context, val, SCPI_REG_QUESE, STB_QES);
        break;
    case SCPI_REG_QUESE:
        regUpdate(context, SCPI_REG_QUES);
        break;
    case SCPI_REG_OPER:
        regUpdateSTB(context, val, SCPI_REG_OPERE, STB_OPS);
        break;
    case SCPI_REG_OPERE:
        regUpdate(context, SCPI_REG_OPER);
        break;


    case SCPI_REG_COUNT:
        /* nothing to do */
        break;
    }

    /* set updated register value */
    context->registers[name] = val;

    if (srq) {
        writeControl(context, SCPI_CTRL_SRQ, SCPI_RegGet(context, SCPI_REG_STB));
    }
}

/**
 * Set register bits
 * @param name - register name
 * @param bits bit mask
 */
void SCPIParser::SCPI_RegSetBits(scpi_t * context, scpi_reg_name_t name, scpi_reg_val_t bits) {
    SCPI_RegSet(context, name, SCPI_RegGet(context, name) | bits);
}

/**
 * Clear register bits
 * @param name - register name
 * @param bits bit mask
 */
void SCPIParser::SCPI_RegClearBits(scpi_t * context, scpi_reg_name_t name, scpi_reg_val_t bits) {
    SCPI_RegSet(context, name, SCPI_RegGet(context, name) & ~bits);
}

/**
 * Clear event register
 * @param context
 */
void SCPIParser::SCPI_EventClear(scpi_t * context) {
    /* TODO */
    SCPI_RegSet(context, SCPI_REG_ESR, 0);
}

/**
 * *CLS - This command clears all status data structures in a device.
 *        For a device which minimally complies with SCPI. (SCPI std 4.1.3.2)
 * @param context
 * @return
 */
scpi_result_t SCPIParser::SCPI_CoreCls(scpi_t * context) {
    SCPI_EventClear(context);
    SCPI_ErrorClear(context);
    SCPI_RegSet(context, SCPI_REG_OPER, 0);
    SCPI_RegSet(context, SCPI_REG_QUES, 0);
    return SCPI_RES_OK;
}

/**
 * *ESE
 * @param context
 * @return
 */
scpi_result_t SCPIParser::SCPI_CoreEse(scpi_t * context) {
    int32_t new_ESE;
    if (SCPI_ParamInt(context, &new_ESE, TRUE)) {
        SCPI_RegSet(context, SCPI_REG_ESE, new_ESE);
    }
    return SCPI_RES_OK;
}

/**
 * *ESE?
 * @param context
 * @return
 */
scpi_result_t SCPIParser::SCPI_CoreEseQ(scpi_t * context) {
    SCPI_ResultInt(context, SCPI_RegGet(context, SCPI_REG_ESE));
    return SCPI_RES_OK;
}

/**
 * *ESR?
 * @param context
 * @return
 */
scpi_result_t SCPIParser::SCPI_CoreEsrQ(scpi_t * context) {
    SCPI_ResultInt(context, SCPI_RegGet(context, SCPI_REG_ESR));
    SCPI_RegSet(context, SCPI_REG_ESR, 0);
    return SCPI_RES_OK;
}

/**
 * *IDN?
 *
 * field1: MANUFACTURE
 * field2: MODEL
 * field4: SUBSYSTEMS REVISIONS
 *
 * example: MANUFACTURE,MODEL,0,01-02-01
 * @param context
 * @return
 */
scpi_result_t SCPIParser::SCPI_CoreIdnQ(scpi_t * context) {
    SCPI_ResultString(context, context->idn[0]);
    SCPI_ResultString(context, context->idn[1]);
    SCPI_ResultString(context, context->idn[2]);
    SCPI_ResultString(context, context->idn[3]);
    return SCPI_RES_OK;
}

/**
 * *OPC
 * @param context
 * @return
 */
scpi_result_t SCPIParser::SCPI_CoreOpc(scpi_t * context) {
    SCPI_RegSetBits(context, SCPI_REG_ESR, ESR_OPC);
    return SCPI_RES_OK;
}

/**
 * *OPC?
 * @param context
 * @return
 */
scpi_result_t SCPIParser::SCPI_CoreOpcQ(scpi_t * context) {
    /* Operation is always completed */
    SCPI_ResultInt(context, 1);
    return SCPI_RES_OK;
}

/**
 * *RST
 * @param context
 * @return
 */
scpi_result_t SCPIParser::SCPI_CoreRst(scpi_t * context) {
    if (context && context->interface && context->interface->reset) {
        return context->interface->reset(context);
    }
    return SCPI_RES_OK;
}

/**
 * *SRE
 * @param context
 * @return
 */
scpi_result_t SCPIParser::SCPI_CoreSre(scpi_t * context) {
    int32_t new_SRE;
    if (SCPI_ParamInt(context, &new_SRE, TRUE)) {
        SCPI_RegSet(context, SCPI_REG_SRE, new_SRE);
    }
    return SCPI_RES_OK;
}

/**
 * *SRE?
 * @param context
 * @return
 */
scpi_result_t SCPIParser::SCPI_CoreSreQ(scpi_t * context) {
    SCPI_ResultInt(context, SCPI_RegGet(context, SCPI_REG_SRE));
    return SCPI_RES_OK;
}

/**
 * *STB?
 * @param context
 * @return
 */
scpi_result_t SCPIParser::SCPI_CoreStbQ(scpi_t * context) {
    SCPI_ResultInt(context, SCPI_RegGet(context, SCPI_REG_STB));
    return SCPI_RES_OK;
}

/**
 * *TST?
 * @param context
 * @return
 */
scpi_result_t SCPIParser::SCPI_CoreTstQ(scpi_t * context) {
    int result = 0;
    if (context && context->interface && context->interface->test) {
        result = context->interface->test(context);
    }
    SCPI_ResultInt(context, result);
    return SCPI_RES_OK;
}

/**
 * *WAI
 * @param context
 * @return
 */
scpi_result_t SCPIParser::SCPI_CoreWai(scpi_t * context) {
    (void) context;
    /* NOP */
    return SCPI_RES_OK;
}






