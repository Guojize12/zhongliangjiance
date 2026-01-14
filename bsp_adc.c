#include "bsp_adc.h"
#include "bsp_timer.h"
#include "mf_adc.h"
#include "fm33lc0xx_fl.h"

/* --------- 配置参数 --------- */

static uint16_t s_weight_threshold = WEIGHT_ADC_THRESHOLD_DEFAULT;   // 检测阈值
static uint16_t s_sample_period_ms = WEIGHT_SAMPLE_PERIOD_MS_DEFAULT; // 检测周期ms
static uint8_t  s_confirm_count    = WEIGHT_INSERT_CONFIRM_COUNT;     // 连续确认次数

/* --------- 内部状态结构体 --------- */

typedef struct {
    struct Timer timer;    // 定时器句柄

    GPIO_Type *ctrl_gpio;  // 控制引脚GPIO组
    uint32_t   ctrl_pin;   // 控制引脚管脚号

    uint32_t   adc_channel;    // 采样ADC通道

    uint8_t    status;     // 状态：0=未插入，1=已插入

    uint8_t    inserted_consecutive; // 连续"插入"计数防抖
    uint16_t   last_adc;   // 最近一次采样原始值

    uint8_t    running;    // 定时器是否在运行
} WeightDetectCtx;

/* 组1：PD0，控制PB9；组2：PD11，控制PB10 */
static WeightDetectCtx g_w1 = {
    .ctrl_gpio   = GPIOB,
    .ctrl_pin    = FL_GPIO_PIN_9,
    .adc_channel = FL_ADC_EXTERNAL_CH10, // PD0
};
static WeightDetectCtx g_w2 = {
    .ctrl_gpio   = GPIOB,
    .ctrl_pin    = FL_GPIO_PIN_10,
    .adc_channel = FL_ADC_EXTERNAL_CH8, // PD11
};

/* --------- 控制引脚操作 --------- */

static inline void weight_power_on(WeightDetectCtx *ctx) {
    FL_GPIO_WritePin(ctx->ctrl_gpio, ctx->ctrl_pin, FL_SET);
}
static inline void weight_power_off(WeightDetectCtx *ctx) {
    FL_GPIO_WritePin(ctx->ctrl_gpio, ctx->ctrl_pin, FL_RESET);
}
static void control_pins_init(void) {
    /* 初始化PB9和PB10为推挽输出，默认拉低 */
    FL_GPIO_InitTypeDef gpio = {0};
    gpio.pin        = FL_GPIO_PIN_9 | FL_GPIO_PIN_10;
    gpio.mode       = FL_GPIO_MODE_OUTPUT;
    gpio.outputType = FL_GPIO_OUTPUT_PUSHPULL;
    gpio.pull       = FL_DISABLE;
    gpio.remapPin   = FL_DISABLE;
    FL_GPIO_Init(GPIOB, &gpio);

    FL_GPIO_WritePin(GPIOB, FL_GPIO_PIN_9, FL_RESET);
    FL_GPIO_WritePin(GPIOB, FL_GPIO_PIN_10, FL_RESET);
}

/* --------- ADC单通道采样，轮询无延时 --------- */
static uint16_t adc_read_single_raw(uint32_t channel) {
    uint16_t result = 0;

    FL_RCC_SetADCPrescaler(FL_RCC_ADC_PSC_DIV1);

    FL_ADC_DisableSequencerChannel(ADC, FL_ADC_ALL_CHANNEL);
    FL_ADC_EnableSequencerChannel(ADC, channel);

    FL_ADC_ClearFlag_EndOfConversion(ADC);
    FL_ADC_Enable(ADC);
    FL_ADC_EnableSWConversion(ADC);

    uint32_t spin = 0;
    while (FL_ADC_IsActiveFlag_EndOfConversion(ADC) == FL_RESET && spin < 100000) {
        spin++;
    }

    if (FL_ADC_IsActiveFlag_EndOfConversion(ADC) == FL_SET) {
        result = FL_ADC_ReadConversionData(ADC) & 0x0FFF;
        FL_ADC_ClearFlag_EndOfConversion(ADC);
    }
    FL_ADC_Disable(ADC);
    FL_ADC_DisableSequencerChannel(ADC, channel);

    return result;
}

/* --------- 事件回调（弱定义，可在app层重写） --------- */
__attribute__((weak)) void BSP_ADC_OnWeight1Inserted(void) {}
__attribute__((weak)) void BSP_ADC_OnWeight1NotInserted(void) {}
__attribute__((weak)) void BSP_ADC_OnWeight2Inserted(void) {}
__attribute__((weak)) void BSP_ADC_OnWeight2NotInserted(void) {}

/* --------- 核心检测逻辑（定时器回调） --------- */

static void weight_timer_cb_w1(void) {
    uint16_t raw = adc_read_single_raw(g_w1.adc_channel);
    g_w1.last_adc = raw;

    if (raw > s_weight_threshold) {
        // 连续"插入"计数+1
        if (g_w1.inserted_consecutive < 255) g_w1.inserted_consecutive++;
        // 达到防抖判定且状态原本为未插入，则切换状态
        if (g_w1.inserted_consecutive >= s_confirm_count && g_w1.status == 0) {
            g_w1.status = 1;
            weight_power_off(&g_w1);
            BSP_TIMER_Stop(&g_w1.timer);
            g_w1.running = 0;
            BSP_ADC_OnWeight1Inserted();
        }
    } else {
        // 非插入时归零，保持供电持续侦测
        g_w1.inserted_consecutive = 0;
        if (g_w1.status != 0) {
            g_w1.status = 0;
            BSP_ADC_OnWeight1NotInserted();
        }
        weight_power_on(&g_w1);
    }
}

static void weight_timer_cb_w2(void) {
    uint16_t raw = adc_read_single_raw(g_w2.adc_channel);
    g_w2.last_adc = raw;

    if (raw > s_weight_threshold) {
        if (g_w2.inserted_consecutive < 255) g_w2.inserted_consecutive++;
        if (g_w2.inserted_consecutive >= s_confirm_count && g_w2.status == 0) {
            g_w2.status = 1;
            weight_power_off(&g_w2);
            BSP_TIMER_Stop(&g_w2.timer);
            g_w2.running = 0;
            BSP_ADC_OnWeight2Inserted();
        }
    } else {
        g_w2.inserted_consecutive = 0;
        if (g_w2.status != 0) {
            g_w2.status = 0;
            BSP_ADC_OnWeight2NotInserted();
        }
        weight_power_on(&g_w2);
    }
}

/* --------- 外部接口实现 --------- */

void BSP_ADC_Init(void) {
    /* ADC底层初始化 */
    MF_ADC_Common_Init();
    MF_ADC_GPIO_Init();
    MF_ADC_Sampling_Init();

    /* 控制引脚初始化 */
    control_pins_init();

    /* 初始化定时器 */
    BSP_TIMER_Init(&g_w1.timer, weight_timer_cb_w1, s_sample_period_ms, s_sample_period_ms);
    BSP_TIMER_Init(&g_w2.timer, weight_timer_cb_w2, s_sample_period_ms, s_sample_period_ms);

    /* 状态归零 */
    g_w1.status = 0; g_w1.inserted_consecutive = 0; g_w1.last_adc = 0; g_w1.running = 0;
    g_w2.status = 0; g_w2.inserted_consecutive = 0; g_w2.last_adc = 0; g_w2.running = 0;

    /* 默认断电，直到调用Start */
    weight_power_off(&g_w1);
    weight_power_off(&g_w2);
}

void BSP_ADC_Weight1_Start(void) {
    g_w1.status = 0;
    g_w1.inserted_consecutive = 0;
    g_w1.last_adc = 0;

    weight_power_on(&g_w1);
    BSP_TIMER_Stop(&g_w1.timer);
    BSP_TIMER_Init(&g_w1.timer, weight_timer_cb_w1, s_sample_period_ms, s_sample_period_ms);
    BSP_TIMER_Start(&g_w1.timer);
    g_w1.running = 1;
}

void BSP_ADC_Weight2_Start(void) {
    g_w2.status = 0;
    g_w2.inserted_consecutive = 0;
    g_w2.last_adc = 0;

    weight_power_on(&g_w2);
    BSP_TIMER_Stop(&g_w2.timer);
    BSP_TIMER_Init(&g_w2.timer, weight_timer_cb_w2, s_sample_period_ms, s_sample_period_ms);
    BSP_TIMER_Start(&g_w2.timer);
    g_w2.running = 1;
}

void BSP_ADC_Weight1_Stop(void) {
    BSP_TIMER_Stop(&g_w1.timer);
    g_w1.running = 0;
    weight_power_off(&g_w1);
}

void BSP_ADC_Weight2_Stop(void) {
    BSP_TIMER_Stop(&g_w2.timer);
    g_w2.running = 0;
    weight_power_off(&g_w2);
}

uint8_t BSP_ADC_Weight1_Status(void) { return g_w1.status; }
uint8_t BSP_ADC_Weight2_Status(void) { return g_w2.status; }

uint16_t BSP_ADC_Weight1_LastADC(void) { return g_w1.last_adc; }
uint16_t BSP_ADC_Weight2_LastADC(void) { return g_w2.last_adc; }

void BSP_ADC_Weight_SetThreshold(uint16_t threshold) { s_weight_threshold = threshold; }
void BSP_ADC_Weight_SetSamplePeriod(uint16_t period_ms) {
    s_sample_period_ms = period_ms;
    if (g_w1.running) BSP_TIMER_Modify_Start(&g_w1.timer, s_sample_period_ms, s_sample_period_ms);
    if (g_w2.running) BSP_TIMER_Modify_Start(&g_w2.timer, s_sample_period_ms, s_sample_period_ms);
}
void BSP_ADC_Weight_SetConfirmCount(uint8_t count) { s_confirm_count = (count == 0) ? 1 : count; }

void BSP_ADC_Weight_StartAll(void) {
    BSP_ADC_Weight1_Start();
    BSP_ADC_Weight2_Start();
}

/*****END OF FILE****/