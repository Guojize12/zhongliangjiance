#include "bsp_adc.h"
#include "bsp_config.h"

#define  BAT_VOL_MIN  3100     //最小电压
#define  BAT_VOL_MAX  4150     //最大电压

static uint16_t g_adc_vol[11]=
{
    4170,4060,3980,3920,3870,3820,3790,3770,3740,3680,3450,
};

static Timer g_timer_adc = {0};

// 0-bat     1-dc_in       2-sun
// 3-        4-            5-
adc_def g_adc_cfg=
{
    .adc_chl={
        FL_ADC_EXTERNAL_CH4,FL_ADC_EXTERNAL_CH3,FL_ADC_EXTERNAL_CH2,
        FL_ADC_EXTERNAL_CH6,FL_ADC_EXTERNAL_CH7,FL_ADC_EXTERNAL_CH10,
    },
    .adc_chl_vol={
        USE_042V,USE_033V,USE_188V,
        USE_033V,USE_033V,USE_033V,
    },
    .delay =20,
};

static uint32_t GetVREF1P2Sample_POLL(void)
{
    uint16_t ADCRdresult;
    uint8_t i = 0;
    FL_RCC_SetADCPrescaler(FL_RCC_ADC_PSC_DIV8);
    FL_VREF_EnableVREFBuffer(VREF);//使能VREF BUFFER
    FL_ADC_DisableSequencerChannel(ADC, FL_ADC_ALL_CHANNEL);//清空打开的通道
    FL_ADC_EnableSequencerChannel(ADC, FL_ADC_INTERNAL_VREF1P2);//通道选择VREF

    FL_ADC_ClearFlag_EndOfConversion(ADC);//清标志
    FL_ADC_Enable(ADC);      // 启动ADC
    FL_ADC_EnableSWConversion(ADC);  // 开始转换

    // 等待转换完成
    while(FL_ADC_IsActiveFlag_EndOfConversion(ADC) == FL_RESET)
    {
        if(i >= 5)
        {
            break;
        }

        i++;
        FL_DelayMs(1);
    }

    FL_ADC_ClearFlag_EndOfConversion(ADC);//清标志
    ADCRdresult = FL_ADC_ReadConversionData(ADC); //获取采样值

    FL_ADC_Disable(ADC);    // 关闭ADC
    FL_ADC_DisableSequencerChannel(ADC, FL_ADC_INTERNAL_VREF1P2);//通道关闭VREF
    FL_VREF_DisableVREFBuffer(VREF);//关闭VREF BUFFER
    // 转换结果
    return ADCRdresult;
}

static uint32_t GetSingleChannelSample_POLL(uint32_t channel)
{
    uint16_t ADCRdresult;
    uint8_t i = 0;
    FL_RCC_SetADCPrescaler(FL_RCC_ADC_PSC_DIV1);
    FL_ADC_DisableSequencerChannel(ADC, FL_ADC_ALL_CHANNEL);//清空打开的通道
    FL_ADC_EnableSequencerChannel(ADC, channel);//通道选择

    FL_ADC_ClearFlag_EndOfConversion(ADC);//清标志
    FL_ADC_Enable(ADC);      // 启动ADC
    FL_ADC_EnableSWConversion(ADC);  // 开始转换

    // 等待转换完成
    while(FL_ADC_IsActiveFlag_EndOfConversion(ADC) == FL_RESET)
    {
        if(i >= 5)
        {
            break;
        }

        i++;
        FL_DelayMs(1);
    }

    FL_ADC_ClearFlag_EndOfConversion(ADC);//清标志
    ADCRdresult = FL_ADC_ReadConversionData(ADC); //获取采样值

    FL_ADC_Disable(ADC);    // 关闭ADC
    FL_ADC_DisableSequencerChannel(ADC, channel);//通道
    // 转换结果
    return ADCRdresult;
}

uint32_t GetSingleChannelVoltage_POLL(uint32_t channel,uint8_t vol)
{
    uint32_t Get122VSample, GetChannelVoltage;
    uint64_t GetVSample;

    Get122VSample = GetVREF1P2Sample_POLL();
    GetVSample = GetSingleChannelSample_POLL(channel);

    GetChannelVoltage = (GetVSample * 3000 * (ADC_VREF)) / (Get122VSample * 4095);

    switch(vol)
    {
    case USE_188V:
        GetChannelVoltage = GetChannelVoltage*(470+100)/100;
        break;
    case USE_139V:
        GetChannelVoltage = GetChannelVoltage*(200+62)/62;
        break;
    case USE_130V:
        GetChannelVoltage = GetChannelVoltage*(200+68)/68;
        break;
    case USE_042V:
        GetChannelVoltage = GetChannelVoltage*(62+200)/200;
        break;
    case USE_090V:
        GetChannelVoltage = GetChannelVoltage*287/100;
        break;
    }
    // 转换结果
    return GetChannelVoltage;
}

uint8_t BSP_ADC_Voltage_Poll(void)
{
    int i,j,k;
    uint8_t ret = 0;

    for(i=0; i<READ_ADC_NUM; i++)
    {
        g_adc_cfg.vol_sub[i][g_adc_cfg.vol_num]= GetSingleChannelVoltage_POLL(g_adc_cfg.adc_chl[i],g_adc_cfg.adc_chl_vol[i]);
    }
    g_adc_cfg.vol_num++;

    if(g_adc_cfg.vol_num >= READ_ADC_TIMES)
    {
        if(FILTER_TIMES*2 < g_adc_cfg.vol_num)
        {
            uint32_t g_voltage_max;
            uint32_t g_voltage_min;
            for(k=0; k<READ_ADC_NUM; k++)
            {
                for(i=0; i<FILTER_TIMES; i++)
                {
                    g_voltage_max = g_adc_cfg.vol_sub[k][0];//赋初值,求最大,放在数组尾

                    for(j=1; j<g_adc_cfg.vol_num-i*2; j++)
                    {
                        if(g_voltage_max>g_adc_cfg.vol_sub[k][j])
                        {
                            g_adc_cfg.vol_sub[k][j-1] =g_adc_cfg.vol_sub[k][j];
                            g_adc_cfg.vol_sub[k][j]=g_voltage_max;
                        }
                        else
                        {
                            g_voltage_max = g_adc_cfg.vol_sub[k][j];
                        }
                    }

                    g_voltage_min = g_adc_cfg.vol_sub[k][0];//赋初值,求最小,放在数组尾

                    for(j=1; j<g_adc_cfg.vol_num-i*2-1; j++)
                    {
                        if(g_voltage_min<g_adc_cfg.vol_sub[k][j])
                        {
                            g_adc_cfg.vol_sub[k][j-1] =g_adc_cfg.vol_sub[k][j];
                            g_adc_cfg.vol_sub[k][j]=g_voltage_min;
                        }
                        else
                        {
                            g_voltage_min = g_adc_cfg.vol_sub[k][j];
                        }
                    }
                }

                g_adc_cfg.vol_sum=0;
                for(i=0; i<g_adc_cfg.vol_num-FILTER_TIMES*2; i++)
                {
                    g_adc_cfg.vol_sum += g_adc_cfg.vol_sub[k][i];
                }
                g_adc_cfg.adc_value[k] = g_adc_cfg.vol_sum;
                g_adc_cfg.vol[k]=g_adc_cfg.vol_sum/(g_adc_cfg.vol_num-FILTER_TIMES*2);

                if(BSP_CONFIG_Show_Get() == 101)
                {
                    LOG("g_adc_cfg.vol[%d]=%lu\n",k,(long unsigned int)g_adc_cfg.vol[k]);
                }

            }
        }

        g_adc_cfg.vol_num=0;
        ret = 1;
    }

    return ret;
}

uint8_t  BSP_ADC_Level_Cal(uint16_t vol,const uint16_t vol_up,const uint16_t vol_down)
{
    uint8_t level=1;
    uint16_t up = vol_up, down = vol_down;
    level = ((vol-down)*10/(up-down));
    return level;
}

uint8_t  BSP_ADC_Level(uint16_t vol)
{
    uint8_t level = 1;
    if(vol>=g_adc_vol[0])
    {
        level = 100;
    }
    else if(vol<g_adc_vol[0] && vol>=g_adc_vol[1])
    {
        level = BSP_ADC_Level_Cal(vol,g_adc_vol[0],g_adc_vol[1]) +90;
    }
    else if(vol<g_adc_vol[1] && vol>=g_adc_vol[2])
    {
        level = BSP_ADC_Level_Cal(vol,g_adc_vol[1],g_adc_vol[2]) +80;
    }
    else if(vol<g_adc_vol[2] && vol>=g_adc_vol[3])
    {
        level = BSP_ADC_Level_Cal(vol,g_adc_vol[2],g_adc_vol[3]) +70;
    }
    else if(vol<g_adc_vol[3] && vol>=g_adc_vol[4])
    {
        level = BSP_ADC_Level_Cal(vol,g_adc_vol[3],g_adc_vol[4]) +60;
    }
    else if(vol<g_adc_vol[4] && vol>=g_adc_vol[5])
    {
        level = BSP_ADC_Level_Cal(vol,g_adc_vol[4],g_adc_vol[5]) +50;
    }
    else if(vol<g_adc_vol[5] && vol>=g_adc_vol[6])
    {
        level = BSP_ADC_Level_Cal(vol,g_adc_vol[5],g_adc_vol[6]) +40;
    }
    else if(vol<g_adc_vol[6] && vol>=g_adc_vol[7])
    {
        level = BSP_ADC_Level_Cal(vol,g_adc_vol[6],g_adc_vol[7]) +30;
    }
    else if(vol<g_adc_vol[7] && vol>=g_adc_vol[8])
    {
        level = BSP_ADC_Level_Cal(vol,g_adc_vol[7],g_adc_vol[8]) +20;
    }
    else if(vol<g_adc_vol[8] && vol>=g_adc_vol[9])
    {
        level = BSP_ADC_Level_Cal(vol,g_adc_vol[8],g_adc_vol[9]) +10;
    }
    else if(vol<g_adc_vol[9] && vol>=g_adc_vol[10])
    {
        level = BSP_ADC_Level_Cal(vol,g_adc_vol[9],g_adc_vol[10]) +1;
    }
    else
    {
        level = 1;
    }

    return level;

}
void BSP_ADC_Timer_Handle(void)
{
    if(BSP_ADC_Voltage_Poll()==1) //ADC 读取
    {
        g_adc_cfg.vol_lev = BSP_ADC_Level(g_adc_cfg.vol[0]);

        if(g_adc_cfg.vol[2]>6000 && g_adc_cfg.vol_lev< 100) //太阳能有电  &&  电量< 100
        {
            // BSP_SENSOR_Status_Tiomeout_Rst(SENSOR_NUM24);
        }
        if(g_adc_cfg.adc_value[3]>20) //重量1检测
        {
            // BSP_SENSOR_Status_Tiomeout_Rst(SENSOR_NUM11);
        }
        if(g_adc_cfg.adc_value[4]>20) //重量2检测
        {
            // BSP_SENSOR_Status_Tiomeout_Rst(SENSOR_NUM12);
        }
        if(g_adc_cfg.vol[5]<500) //风速插入检测 0.5v
        {
            // BSP_SENSOR_Status_Tiomeout_Rst(SENSOR_NUM05);
        }


        if(BSP_CONFIG_Show_Get() == 101)
        {
            LOG("bat->vol[%lu]mv:lev[%lu]%%,sun->vol[%lu]mv\n",g_adc_cfg.vol[0],g_adc_cfg.vol_lev,g_adc_cfg.vol[2]);
        }
    }
}

void BSP_ADC_Init(void)
{
    BSP_TIMER_Init(&g_timer_adc,BSP_ADC_Timer_Handle,TIMEOUT_100MS,TIMEOUT_100MS);
    BSP_TIMER_Start(&g_timer_adc);
}
/*****END OF FILE****/

