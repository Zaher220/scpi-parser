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

#include "units.h"
#include <utils_private.h>


/* scpi commands */
typedef enum _scpi_result_t {
    SCPI_RES_OK = 1,
    SCPI_RES_ERR = -1
}scpi_result_t;

/* scpi units */
enum scpi_unit_t {
    SCPI_UNIT_NONE,
    SCPI_UNIT_VOLT,
    SCPI_UNIT_AMPER,
    SCPI_UNIT_OHM,
    SCPI_UNIT_HERTZ,
    SCPI_UNIT_CELSIUS,
    SCPI_UNIT_SECONDS,
    SCPI_UNIT_DISTANCE
};

struct scpi_unit_def_t {
    const char * name;
    scpi_unit_t unit;
    double mult;
};
#define SCPI_UNITS_LIST_END       {NULL, SCPI_UNIT_NONE, 0}

enum scpi_special_number_t {
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

struct scpi_special_number_def_t {
    const char * name;
    scpi_special_number_t type;
};
#define SCPI_SPECIAL_NUMBERS_LIST_END   {NULL, SCPI_NUM_NUMBER}

struct scpi_number_t {
    double value;
    scpi_unit_t unit;
    scpi_special_number_t type;
};

/* IEEE 488.2 registers */
enum scpi_reg_name_t {
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

enum scpi_ctrl_name_t {
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

typedef uint16_t scpi_reg_val_t;


class SCPIParser : public QObject
{
    Q_OBJECT

public:
    explicit SCPIParser(QObject *parent = 0);

signals:

public slots:

public:

    typedef bool scpi_bool_t;
    /* typedef enum { FALSE = 0, TRUE } scpi_bool_t; */


    /* scpi interface */
    struct scpi_buffer_t {
        size_t length;
        size_t position;
        char * data;
    };


    typedef size_t(SCPIParser::*scpi_write_t)(const char * data, size_t len);
    typedef scpi_result_t(SCPIParser::*scpi_write_control_t)(scpi_ctrl_name_t ctrl, scpi_reg_val_t val);
    typedef int (SCPIParser::*scpi_error_callback_t)(int_fast16_t error);

    typedef scpi_result_t(SCPIParser::*scpi_command_callback_t)();

    /* scpi error queue */
    typedef void * scpi_error_queue_t;


    struct scpi_command_t {
        const char * pattern;
        scpi_command_callback_t callback;
    };

    struct scpi_param_list_t {
        const scpi_command_t * cmd;
        const char * parameters;
        size_t length;
    };
#define SCPI_CMD_LIST_END       {NULL, NULL, }

    struct scpi_interface_t {
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
    typedef struct _scpi_t scpi_t;

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

    void SCPI_Init();
    int SCPI_Input(const char * data, size_t len);


    int SCPI_Parse(char * data, size_t len);
    size_t SCPI_ResultString(const char * data);
    size_t SCPI_ResultInt(int32_t val);
    size_t SCPI_ResultDouble(double val);
    size_t SCPI_ResultText(const char * data);
    size_t SCPI_ResultBool(scpi_bool_t val);

    scpi_bool_t SCPI_ParamInt(int32_t * value, scpi_bool_t mandatory);
    scpi_bool_t SCPI_ParamDouble(double * value, scpi_bool_t mandatory);
    scpi_bool_t SCPI_ParamString(const char ** value, size_t * len, scpi_bool_t mandatory);
    scpi_bool_t SCPI_ParamText(const char ** value, size_t * len, scpi_bool_t mandatory);
    scpi_bool_t SCPI_ParamBool(scpi_bool_t * value, scpi_bool_t mandatory);
    scpi_bool_t SCPI_ParamChoice(const char * options[], int32_t * value, scpi_bool_t mandatory);

    scpi_bool_t translateSpecialNumber(const scpi_special_number_def_t * specs, const char * str, size_t len, scpi_number_t * value);
    const char * translateSpecialNumberInverse(const scpi_special_number_def_t * specs, scpi_special_number_t type);
    const scpi_unit_def_t * translateUnit(const scpi_unit_def_t * units, const char * unit, size_t len);
    scpi_bool_t transformNumber(const char * unit, size_t len, scpi_number_t * value);
    const char * translateUnitInverse(const scpi_unit_def_t * units, const scpi_unit_t unit);



    size_t writeData(const char * data, size_t len);

    int flushData() ;
    size_t writeDelimiter();
    size_t writeNewLine();
    void processCommand();
    scpi_bool_t findCommand(const char * cmdline_ptr, size_t cmdline_len, size_t cmd_len);
    void SCPI_ErrorAddInternal(int16_t err);
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

    void paramSkipBytes(size_t num);
    void paramSkipWhitespace();
    scpi_bool_t paramNext(scpi_bool_t mandatory);

    //error
    void SCPI_ErrorInit();
    void SCPI_ErrorClear();
    int16_t SCPI_ErrorPop();
    void SCPI_ErrorPush(int16_t err);
    int32_t SCPI_ErrorCount();
    const char * SCPI_ErrorTranslate(int16_t err);

    //debug
    scpi_bool_t SCPI_DebugCommand();

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



    scpi_bool_t SCPI_ParamNumber(scpi_number_t * value, scpi_bool_t mandatory);
    size_t SCPI_NumberToStr(scpi_number_t * value, char * str, size_t len);



    //ieee
    scpi_result_t SCPI_CoreCls();
    scpi_result_t SCPI_CoreEse();
    scpi_result_t SCPI_CoreEseQ();
    scpi_result_t SCPI_CoreEsrQ();
    scpi_result_t SCPI_CoreIdnQ();
    scpi_result_t SCPI_CoreOpc();
    scpi_result_t SCPI_CoreOpcQ();
    scpi_result_t SCPI_CoreRst();
    scpi_result_t SCPI_CoreSre();
    scpi_result_t SCPI_CoreSreQ();
    scpi_result_t SCPI_CoreStbQ();
    scpi_result_t SCPI_CoreTstQ();
    scpi_result_t SCPI_CoreWai();
    void SCPI_EventClear() ;


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


    scpi_reg_val_t SCPI_RegGet(scpi_reg_name_t name);
    void SCPI_RegSet(scpi_reg_name_t name, scpi_reg_val_t val);
    void SCPI_RegSetBits(scpi_reg_name_t name, scpi_reg_val_t bits);
    void SCPI_RegClearBits(scpi_reg_name_t name, scpi_reg_val_t bits);
    void regUpdate(scpi_reg_name_t name);
    void regUpdateSTB(scpi_reg_val_t val, scpi_reg_name_t mask, scpi_reg_val_t stbBits);
    size_t writeControl(scpi_ctrl_name_t ctrl, scpi_reg_val_t val);


    scpi_command_t scpi_commands[3] = {
        {"*CLS", &SCPIParser::SCPI_CoreCls,},

        SCPI_CMD_LIST_END
    };
    size_t SCPI_Write(const char * data, size_t len);
    int SCPI_Error(int_fast16_t err);
    scpi_result_t SCPI_Control(scpi_ctrl_name_t ctrl, scpi_reg_val_t val);
    scpi_result_t SCPI_Reset();
    scpi_result_t SCPI_Test();
    scpi_result_t SCPI_Flush();

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

    scpi_t context = {
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
};

#endif // SCPIPARSER_H
