
#define B_BLACK "40"
#define B_RED "41"
#define B_GREEN "42"
#define B_YELLOW "43"
#define B_BLUE "44"
#define B_PURPLE "45"
#define B_DARKGREEN "46"
#define B_WHITE "47"

#define F_BLACK "30"
#define F_RED "31"
#define F_GREEN "32"
#define F_YELLOW "33"
#define F_BLUE "34"
#define F_PURPLE "35"
#define F_DARKGREEN "36"
#define F_WHITE "37"
#define COLOR_PREFIX "\033["
#define COLOR_ENDFIX "\033[0m"
//#define COLOR_TEXT(BC, FC, TEXT) COLOR_PREFIX(1) ##BC##FC
#define LOG_COLOR(B_COLOR, F_COLOR, type)                                                                                                            \
    do                                                                                                                                               \
    {                                                                                                                                                \
        printf("FILE:\t%s\tLINE:\t%d\tDATE:\t%s %s\tMSG:\033[%d:%d\t%s\033[0m\r\n", __FILE__, __LINE__, __DATE__, __TIME__, B_COLOR, F_COLOR, type); \
    } while (0)
#define LOG_NORMAL(TAG, BC, FC, MSG)                                                                        \
    do                                                                                                      \
    {                                                                                                       \
        printf("%s %s:%s TAG:%7s\t%sFILE:%10s\tLINE:%d\tDATE:%s-%s\tMSG:%s\r\n",                       \
               COLOR_PREFIX, BC, FC, TAG, COLOR_ENDFIX, __FILE__, __LINE__, __DATE__, __TIME__, MSG); \
    } while (0)

#define LOGI(msg) LOG_NORMAL("INFO", B_GREEN, B_WHITE, msg)
#define LOGD(msg) LOG_NORMAL("DEBUG", B_GREEN, B_WHITE, msg)
#define LOGW(msg) LOG_NORMAL("WARN", B_GREEN, B_WHITE, msg)
#define LOGE(msg) LOG_NORMAL("ERROR", B_GREEN, B_WHITE, msg)