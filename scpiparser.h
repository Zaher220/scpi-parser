#ifndef SCPIPARSER_H
#define SCPIPARSER_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <QObject>

#include "ieee488.h"
#include "error.h"
#include "constants.h"
#include "minimal.h"
#include "units.h"
#include <utils_private.h>











class SCPIParser : public QObject
{
    Q_OBJECT

public:
    explicit SCPIParser(QObject *parent = 0);

signals:

public slots:

private:





    typedef bool scpi_bool_t;
        /* typedef enum { FALSE = 0, TRUE } scpi_bool_t; */

        /* IEEE 488.2 registers */
        enum _scpi_reg_name_t {
            SCPI_REG_STB = 0, /* Status Byte */
            SCPI_REG_SRE,     /* Service Request Enable Register */
            SCPI_REG_ESR,     /* Standard Event Status Register (ESR, SESR) */
            SCPI_REG_ESE,     /* Event Status Enable Register */
            SCPI_REG_OPER,    /* OPERation Status Register */
            SCPI_REG_OPERE,   /* OPERation Status Enable Register */
            SCPI_REG_QUES,    /* QUEStionable status register */
            SCPI_REG_QUESE,   /* QUEStionable status Enable Register */

            /* last definition - number of registers */
            SCPI_REG_COUNT
        };
        typedef enum _scpi_reg_name_t scpi_reg_name_t;

        enum _scpi_ctrl_name_t {
            SCPI_CTRL_SRQ = 1, /* service request */
            SCPI_CTRL_GTL,     /* Go to local */
            SCPI_CTRL_SDC,     /* Selected device clear */
            SCPI_CTRL_PPC,     /* Parallel poll configure */
            SCPI_CTRL_GET,     /* Group execute trigger */
            SCPI_CTRL_TCT,     /* Take control */
            SCPI_CTRL_LLO,     /* Device clear */
            SCPI_CTRL_DCL,     /* Local lockout */
            SCPI_CTRL_PPU,     /* Parallel poll unconfigure */
            SCPI_CTRL_SPE,     /* Serial poll enable */
            SCPI_CTRL_SPD,     /* Serial poll disable */
            SCPI_CTRL_MLA,     /* My local address */
            SCPI_CTRL_UNL,     /* Unlisten */
            SCPI_CTRL_MTA,     /* My talk address */
            SCPI_CTRL_UNT,     /* Untalk */
            SCPI_CTRL_MSA      /* My secondary address */
        };
        typedef enum _scpi_ctrl_name_t scpi_ctrl_name_t;

        typedef uint16_t scpi_reg_val_t;

        /* scpi commands */
        enum _scpi_result_t {
            SCPI_RES_OK = 1,
            SCPI_RES_ERR = -1
        };
        typedef enum _scpi_result_t scpi_result_t;

        typedef struct _scpi_command_t scpi_command_t;

        struct _scpi_param_list_t {
            const scpi_command_t * cmd;
            const char * parameters;
            size_t length;
        };
        #define SCPI_CMD_LIST_END       {NULL, NULL, }
        typedef struct _scpi_param_list_t scpi_param_list_t;

        /* scpi interface */
        typedef struct _scpi_t scpi_t;
        typedef struct _scpi_interface_t scpi_interface_t;

        struct _scpi_buffer_t {
            size_t length;
            size_t position;
            char * data;
        };
        typedef struct _scpi_buffer_t scpi_buffer_t;

        typedef size_t(*scpi_write_t)(scpi_t * context, const char * data, size_t len);
        typedef scpi_result_t(*scpi_write_control_t)(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val);
        typedef int (*scpi_error_callback_t)(scpi_t * context, int_fast16_t error);

        typedef scpi_result_t(*scpi_command_callback_t)(scpi_t *);

        /* scpi error queue */
        typedef void * scpi_error_queue_t;

        /* scpi units */
        enum _scpi_unit_t {
            SCPI_UNIT_NONE,
            SCPI_UNIT_VOLT,
            SCPI_UNIT_AMPER,
            SCPI_UNIT_OHM,
            SCPI_UNIT_HERTZ,
            SCPI_UNIT_CELSIUS,
            SCPI_UNIT_SECONDS,
            SCPI_UNIT_DISTANCE
        };
        typedef enum _scpi_unit_t scpi_unit_t;

        struct _scpi_unit_def_t {
            const char * name;
            scpi_unit_t unit;
            double mult;
        };
        #define SCPI_UNITS_LIST_END       {NULL, SCPI_UNIT_NONE, 0}
        typedef struct _scpi_unit_def_t scpi_unit_def_t;

        enum _scpi_special_number_t {
            SCPI_NUM_NUMBER,
            SCPI_NUM_MIN,
            SCPI_NUM_MAX,
            SCPI_NUM_DEF,
            SCPI_NUM_UP,
            SCPI_NUM_DOWN,
            SCPI_NUM_NAN,
            SCPI_NUM_INF,
            SCPI_NUM_NINF
        };
        typedef enum _scpi_special_number_t scpi_special_number_t;

        struct _scpi_special_number_def_t {
            const char * name;
            scpi_special_number_t type;
        };
        #define SCPI_SPECIAL_NUMBERS_LIST_END   {NULL, SCPI_NUM_NUMBER}
        typedef struct _scpi_special_number_def_t scpi_special_number_def_t;

        struct _scpi_number_t {
            double value;
            scpi_unit_t unit;
            scpi_special_number_t type;
        };
        typedef struct _scpi_number_t scpi_number_t;

        struct _scpi_command_t {
            const char * pattern;
            scpi_command_callback_t callback;
        };

        struct _scpi_interface_t {
            scpi_error_callback_t error;
            scpi_write_t write;
            scpi_write_control_t control;
            scpi_command_callback_t flush;
            scpi_command_callback_t reset;
            scpi_command_callback_t test;
        };

        struct _scpi_t {
            const scpi_command_t * cmdlist;
            scpi_buffer_t buffer;
            scpi_param_list_t paramlist;
            scpi_interface_t * interface;
            int_fast16_t output_count;
            int_fast16_t input_count;
            scpi_bool_t cmd_error;
            scpi_error_queue_t error_queue;
            scpi_reg_val_t * registers;
            const scpi_unit_def_t * units;
            const scpi_special_number_def_t * special_numbers;
            void * user_context;
            const char * idn[4];
        };
















#define FIFO_SIZE 16

struct _fifo_t {
    int16_t wr;
    int16_t rd;
    int16_t size;
    int16_t data[FIFO_SIZE];
};
typedef struct _fifo_t fifo_t;

#define SCPI_DEBUG_COMMAND(a)

struct error_reg {
    int16_t from;
    int16_t to;
    scpi_reg_val_t bit;
};

#define ERROR_DEFS_N	8







void SCPI_Init(scpi_t * context);
int SCPI_Input(scpi_t * context, const char * data, size_t len);


    int SCPI_Parse(scpi_t * context, char * data, size_t len);
    size_t SCPI_ResultString(scpi_t * context, const char * data);
    size_t SCPI_ResultInt(scpi_t * context, int32_t val);
    size_t SCPI_ResultDouble(scpi_t * context, double val);
    size_t SCPI_ResultText(scpi_t * context, const char * data);
    size_t SCPI_ResultBool(scpi_t * context, scpi_bool_t val);

    scpi_bool_t SCPI_ParamInt(scpi_t * context, int32_t * value, scpi_bool_t mandatory);
    scpi_bool_t SCPI_ParamDouble(scpi_t * context, double * value, scpi_bool_t mandatory);
    scpi_bool_t SCPI_ParamString(scpi_t * context, const char ** value, size_t * len, scpi_bool_t mandatory);
    scpi_bool_t SCPI_ParamText(scpi_t * context, const char ** value, size_t * len, scpi_bool_t mandatory);
    scpi_bool_t SCPI_ParamBool(scpi_t * context, scpi_bool_t * value, scpi_bool_t mandatory);
    scpi_bool_t SCPI_ParamChoice(scpi_t * context, const char * options[], int32_t * value, scpi_bool_t mandatory);

    scpi_bool_t translateSpecialNumber(const scpi_special_number_def_t * specs, const char * str, size_t len, scpi_number_t * value);
    const char * translateSpecialNumberInverse(const scpi_special_number_def_t * specs, scpi_special_number_t type);
    const scpi_unit_def_t * translateUnit(const scpi_unit_def_t * units, const char * unit, size_t len);
    scpi_bool_t transformNumber(scpi_t * context, const char * unit, size_t len, scpi_number_t * value);
    const char * translateUnitInverse(const scpi_unit_def_t * units, const scpi_unit_t unit);



    size_t writeData(scpi_t * context, const char * data, size_t len);

    int flushData(scpi_t * context) ;
    size_t writeDelimiter(scpi_t * context);
    size_t writeNewLine(scpi_t * context);
    void processCommand(scpi_t * context);
    scpi_bool_t findCommand(scpi_t * context, const char * cmdline_ptr, size_t cmdline_len, size_t cmd_len);
    void SCPI_ErrorAddInternal(scpi_t * context, int16_t err);
    fifo_t local_error_queue;

    struct error_reg errs[ERROR_DEFS_N] = {
    {-100, -199, ESR_CER}, /* Command error (e.g. syntax error) ch 21.8.9    */
    {-200, -299, ESR_EER}, /* Execution Error (e.g. range error) ch 21.8.10  */
    {-300, -399, ESR_DER}, /* Device specific error -300, -399 ch 21.8.11    */
    {-400, -499, ESR_QER}, /* Query error -400, -499 ch 21.8.12              */
    {-500, -599, ESR_PON}, /* Power on event -500, -599 ch 21.8.13           */
    {-600, -699, ESR_URQ}, /* User Request Event -600, -699 ch 21.8.14       */
    {-700, -799, ESR_REQ}, /* Request Control Event -700, -799 ch 21.8.15    */
    {-800, -899, ESR_OPC}, /* Operation Complete Event -800, -899 ch 21.8.16 */
};



    size_t cmdTerminatorPos(const char * cmd, size_t len);
    size_t cmdlineSeparatorPos(const char * cmd, size_t len);
    const char * cmdlineSeparator(const char * cmd, size_t len);
    const char * cmdlineTerminator(const char * cmd, size_t len);
    size_t skipCmdLine(const char * cmd, size_t len);

    void paramSkipBytes(scpi_t * context, size_t num);
    void paramSkipWhitespace(scpi_t * context);
    scpi_bool_t paramNext(scpi_t * context, scpi_bool_t mandatory);




    //error
    void SCPI_ErrorInit(scpi_t * context);
    void SCPI_ErrorClear(scpi_t * context);
    int16_t SCPI_ErrorPop(scpi_t * context);
    void SCPI_ErrorPush(scpi_t * context, int16_t err);
    int32_t SCPI_ErrorCount(scpi_t * context);
    const char * SCPI_ErrorTranslate(int16_t err);

    //debug
    scpi_bool_t SCPI_DebugCommand(scpi_t * context);


    //fifo


    void fifo_init(fifo_t * fifo);
    void fifo_clear(fifo_t * fifo);
    scpi_bool_t fifo_add(fifo_t * fifo, int16_t value);
    scpi_bool_t fifo_remove(fifo_t * fifo, int16_t * value);
    scpi_bool_t fifo_count(fifo_t * fifo, int16_t * value);


    //units
    //    extern const scpi_unit_def_t scpi_units_def[];
    //    extern const scpi_special_number_def_t scpi_special_numbers_def[];


    /*
     * multipliers IEEE 488.2-1992 tab 7-2
     * 1E18         EX
     * 1E15         PE
     * 1E12         T
     * 1E9          G
     * 1E6          MA (use M for OHM and HZ)
     * 1E3          K
     * 1E-3         M (disaalowed for OHM and HZ)
     * 1E-6         U
     * 1E-9         N
     * 1E-12        P
     * 1E-15        F
     * 1E-18        A
     */

    /*
     * units definition IEEE 488.2-1992 tab 7-1
     */
    const scpi_unit_def_t scpi_units_def[24] = {
        /* voltage */
        {/* name */ "UV",   /* unit */ SCPI_UNIT_VOLT,      /* mult */ 1e-6},
        {/* name */ "MV",   /* unit */ SCPI_UNIT_VOLT,      /* mult */ 1e-3},
        {/* name */ "V",    /* unit */ SCPI_UNIT_VOLT,      /* mult */ 1},
        {/* name */ "KV",   /* unit */ SCPI_UNIT_VOLT,      /* mult */ 1e3},

        /* current */
        {/* name */ "UA",   /* unit */ SCPI_UNIT_AMPER,     /* mult */ 1e-6},
        {/* name */ "MA",   /* unit */ SCPI_UNIT_AMPER,     /* mult */ 1e-3},
        {/* name */ "A",    /* unit */ SCPI_UNIT_AMPER,     /* mult */ 1},
        {/* name */ "KA",   /* unit */ SCPI_UNIT_AMPER,     /* mult */ 1e3},

        /* resistance */
        {/* name */ "OHM",  /* unit */ SCPI_UNIT_OHM,       /* mult */ 1},
        {/* name */ "KOHM", /* unit */ SCPI_UNIT_OHM,       /* mult */ 1e3},
        {/* name */ "MOHM", /* unit */ SCPI_UNIT_OHM,       /* mult */ 1e6},

        /* frequency */
        {/* name */ "HZ",   /* unit */ SCPI_UNIT_HERTZ,     /* mult */ 1},
        {/* name */ "KHZ",  /* unit */ SCPI_UNIT_HERTZ,     /* mult */ 1e3},
        {/* name */ "MHZ",  /* unit */ SCPI_UNIT_HERTZ,     /* mult */ 1e6},
        {/* name */ "GHZ",  /* unit */ SCPI_UNIT_HERTZ,     /* mult */ 1e9},

        /* temperature */
        {/* name */ "CEL",  /* unit */ SCPI_UNIT_CELSIUS,   /* mult */ 1},

        /* time */
        {/* name */ "PS",   /* unit */ SCPI_UNIT_SECONDS,   /* mult */ 1e-12},
        {/* name */ "NS",   /* unit */ SCPI_UNIT_SECONDS,   /* mult */ 1e-9},
        {/* name */ "US",   /* unit */ SCPI_UNIT_SECONDS,   /* mult */ 1e-6},
        {/* name */ "MS",   /* unit */ SCPI_UNIT_SECONDS,   /* mult */ 1e-3},
        {/* name */ "S",    /* unit */ SCPI_UNIT_SECONDS,   /* mult */ 1},
        {/* name */ "MIN",  /* unit */ SCPI_UNIT_SECONDS,   /* mult */ 60},
        {/* name */ "HR",   /* unit */ SCPI_UNIT_SECONDS,   /* mult */ 3600},

        SCPI_UNITS_LIST_END,
    };

    /*
     * Special number values definition
     */
    const scpi_special_number_def_t scpi_special_numbers_def[9] = {
        {/* name */ "MINimum",      /* type */ SCPI_NUM_MIN},
        {/* name */ "MAXimum",      /* type */ SCPI_NUM_MAX},
        {/* name */ "DEFault",      /* type */ SCPI_NUM_DEF},
        {/* name */ "UP",           /* type */ SCPI_NUM_UP},
        {/* name */ "DOWN",         /* type */ SCPI_NUM_DOWN},
        {/* name */ "NAN",          /* type */ SCPI_NUM_NAN},
        {/* name */ "INFinity",     /* type */ SCPI_NUM_INF},
        {/* name */ "NINF",         /* type */ SCPI_NUM_NINF},
        SCPI_SPECIAL_NUMBERS_LIST_END,
    };



    scpi_bool_t SCPI_ParamNumber(scpi_t * context, scpi_number_t * value, scpi_bool_t mandatory);
    size_t SCPI_NumberToStr(scpi_t * context, scpi_number_t * value, char * str, size_t len);



    //ieee
    scpi_result_t SCPI_CoreCls(scpi_t * context);
    scpi_result_t SCPI_CoreEse(scpi_t * context);
    scpi_result_t SCPI_CoreEseQ(scpi_t * context);
    scpi_result_t SCPI_CoreEsrQ(scpi_t * context);
    scpi_result_t SCPI_CoreIdnQ(scpi_t * context);
    scpi_result_t SCPI_CoreOpc(scpi_t * context);
    scpi_result_t SCPI_CoreOpcQ(scpi_t * context);
    scpi_result_t SCPI_CoreRst(scpi_t * context);
    scpi_result_t SCPI_CoreSre(scpi_t * context);
    scpi_result_t SCPI_CoreSreQ(scpi_t * context);
    scpi_result_t SCPI_CoreStbQ(scpi_t * context);
    scpi_result_t SCPI_CoreTstQ(scpi_t * context);
    scpi_result_t SCPI_CoreWai(scpi_t * context);
    void SCPI_EventClear(scpi_t * context) ;


#define STB_R01 0x01    /* Not used */
#define STB_PRO 0x02    /* Protection Event Flag */
#define STB_QMA 0x04    /* Error/Event queue message available */
#define STB_QES 0x08    /* Questionable status */
#define STB_MAV 0x10    /* Message Available */
#define STB_ESR 0x20    /* Standard Event Status Register */
#define STB_SRQ 0x40    /* Service Request */
#define STB_OPS 0x80    /* Operation Status Flag */


#define ESR_OPC 0x01    /* Operation complete */
#define ESR_REQ 0x02    /* Request Control */
#define ESR_QER 0x04    /* Query Error */
#define ESR_DER 0x08    /* Device Dependent Error */
#define ESR_EER 0x10    /* Execution Error (e.g. range error) */
#define ESR_CER 0x20    /* Command error (e.g. syntax error) */
#define ESR_URQ 0x40    /* User Request */
#define ESR_PON 0x80    /* Power On */


    scpi_reg_val_t SCPI_RegGet(scpi_t * context, scpi_reg_name_t name);
    void SCPI_RegSet(scpi_t * context, scpi_reg_name_t name, scpi_reg_val_t val);
    void SCPI_RegSetBits(scpi_t * context, scpi_reg_name_t name, scpi_reg_val_t bits);
    void SCPI_RegClearBits(scpi_t * context, scpi_reg_name_t name, scpi_reg_val_t bits);
    void regUpdate(scpi_t * context, scpi_reg_name_t name);
    void regUpdateSTB(scpi_t * context, scpi_reg_val_t val, scpi_reg_name_t mask, scpi_reg_val_t stbBits);
    size_t writeControl(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val);








    scpi_command_t scpi_commands[3] = {
        /* {"pattern", callback} *

                /* IEEE Mandated Commands (SCPI std V1999.0 4.1.1) */
        {"*CLS", &SCPIParser::SCPI_CoreCls,},
        /* {"*ESE", SCPI_CoreEse,},
            {"*ESE?", SCPI_CoreEseQ,},
            {"*ESR?", SCPI_CoreEsrQ,},
            {"*IDN?", SCPI_CoreIdnQ,},
            {"*OPC", SCPI_CoreOpc,},
            {"*OPC?", SCPI_CoreOpcQ,},
            {"*RST", SCPI_CoreRst,},
            {"*SRE", SCPI_CoreSre,},
            {"*SRE?", SCPI_CoreSreQ,},
            {"*STB?", SCPI_CoreStbQ,},
            {"*TST?", SCPI_CoreTstQ,},
            {"*WAI", SCPI_CoreWai,},*/

        /* Required SCPI commands (SCPI std V1999.0 4.2.1) */
        /*{"SYSTem:ERRor?", SCPI_SystemErrorNextQ,},
            {"SYSTem:ERRor:NEXT?", SCPI_SystemErrorNextQ,},
            {"SYSTem:ERRor:COUNt?", SCPI_SystemErrorCountQ,},
            {"SYSTem:VERSion?", SCPI_SystemVersionQ,},

            //{"STATus:OPERation?", scpi_stub_callback,},
            //{"STATus:OPERation:EVENt?", scpi_stub_callback,},
            //{"STATus:OPERation:CONDition?", scpi_stub_callback,},
            //{"STATus:OPERation:ENABle", scpi_stub_callback,},
            //{"STATus:OPERation:ENABle?", scpi_stub_callback,},

            {"STATus:QUEStionable?", SCPI_StatusQuestionableEventQ,},
            {"STATus:QUEStionable:EVENt?", SCPI_StatusQuestionableEventQ,},
            //{"STATus:QUEStionable:CONDition?", scpi_stub_callback,},
            {"STATus:QUEStionable:ENABle", SCPI_StatusQuestionableEnable,},
            {"STATus:QUEStionable:ENABle?", SCPI_StatusQuestionableEnableQ,},

            {"STATus:PRESet", SCPI_StatusPreset,},*/

        /* DMM */
        /*{"MEASure:VOLTage:DC?", DMM_MeasureVoltageDcQ,},
            {"CONFigure:VOLTage:DC", DMM_ConfigureVoltageDc,},
            {"MEASure:VOLTage:DC:RATio?", SCPI_StubQ,},
            {"MEASure:VOLTage:AC?", SCPI_StubQ,},
            {"MEASure:CURRent:DC?", SCPI_StubQ,},
            {"MEASure:CURRent:AC?", SCPI_StubQ,},
            {"MEASure:RESistance?", SCPI_StubQ,},
            {"MEASure:FRESistance?", SCPI_StubQ,},
            {"MEASure:FREQuency?", SCPI_StubQ,},
            {"MEASure:PERiod?", SCPI_StubQ,},

            {"SYSTem:COMMunication:TCPIP:CONTROL?", SCPI_SystemCommTcpipControlQ,},*/

        SCPI_CMD_LIST_END
    };

    scpi_interface_t scpi_interface = {
        /* error */ &SCPIParser::SCPI_Error,
        /* write */ &SCPIParser::SCPI_Write,
        /* control */ &SCPIParser::SCPI_Control,
        /* flush */ &SCPIParser::SCPI_Flush,
        /* reset */ &SCPIParser::SCPI_Reset,
        /* test */ &SCPIParser::SCPI_Test,
    };

#define SCPI_INPUT_BUFFER_LENGTH 256
    char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];

    scpi_reg_val_t scpi_regs[SCPI_REG_COUNT];


    scpi_t scpi_context = {
        /* cmdlist */ scpi_commands,
        /* buffer */ { /* length */ SCPI_INPUT_BUFFER_LENGTH, /* position */ 0,  /* data */ scpi_input_buffer, },
        /* paramlist */ { /* cmd */ NULL, /* parameters */ NULL, /* length */ 0, },
        /* interface */ &scpi_interface,
        /* output_count */ 0,
        /* input_count */ 0,
        /* cmd_error */ FALSE,
        /* error_queue */ NULL,
        /* registers */ scpi_regs,
        /* units */ scpi_units_def,
        /* special_numbers */ scpi_special_numbers_def,
        /* user_context */ NULL,
        /* idn */ {"MANUFACTURE", "INSTR2013", NULL, "01-02"},
    };

    size_t SCPI_Write(scpi_t * context, const char * data, size_t len);
    int SCPI_Error(scpi_t * context, int_fast16_t err);
    scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val);
    scpi_result_t SCPI_Reset(scpi_t * context);
    scpi_result_t SCPI_Test(scpi_t * context);
    scpi_result_t SCPI_Flush(scpi_t * context);


};

#endif // SCPIPARSER_H
