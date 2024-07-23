#include <stdio.h>
#include <stdlib.h>
//#include "main.h"
//#include "string.h"
//#include "lvgl/lvgl.h" lvgl的头文件
#include <lv_drivers/win32drv/win32drv.h>//lvgl针对windows平台的头文件
#include <Windows.h>                //需要包含该头文件


//控件对象
lv_obj_t* obj;
//按键启动
lv_obj_t* button1;
//按键重置
lv_obj_t* button2;
//按键暂停/恢复
lv_obj_t* button3;
//按键退出
lv_obj_t* button4;
//时间输入框
lv_obj_t* text1;
//定时器 更新显示
lv_timer_t* timer;
//定时器 报警灯闪烁
lv_timer_t* timer1;
//报警灯
lv_obj_t* led1;
//运行指示灯
lv_obj_t* led2;
//时间显示控件
lv_obj_t* label1;
//输入指示显示控件
lv_obj_t* label2;
//单选框 毫秒
lv_obj_t* check1;
//单选框 微秒
lv_obj_t* check2;
//单选框 倒序
lv_obj_t* check3;
//单选框 顺序
lv_obj_t* check4;
//按键暂停/恢复 的文本
lv_obj_t* button3_label;

//选择框样式存储
static lv_style_t style_radio;
static lv_style_t style_radio_chk;

//模式存储
//0毫秒 1微秒
static uint32_t active_index_1 = 0;
//0倒计时 1顺计时
static uint32_t active_index_2 = 0;

//灯/提示音 闪烁运行指示
bool bell_run = false;
//暂停计时
bool pause = false;
//灯/提示音 当前状态
bool bell_on = false;
//提示音常响
bool bell_all = false;
//计时器是否工作
bool timer_start = false;

//计时目标
long all_time = 0;
//计时当前值
long count = 0;

void text_set();
void clear_down();
void show_message(char* title);

//单选按键事件回调
static void radio_event_handler(lv_event_t* e)//check_box
{
    uint32_t* active_id = lv_event_get_user_data(e);
    lv_obj_t* cont = lv_event_get_current_target(e);
    lv_obj_t* act_cb = lv_event_get_target(e);
    lv_obj_t* old_cb = lv_obj_get_child(cont, *active_id);
    /*Do nothing if the container was clicked*/
    if (act_cb == cont) return;
    lv_obj_clear_state(old_cb, LV_STATE_CHECKED);   /*Uncheck the previous radio button*/
    lv_obj_add_state(act_cb, LV_STATE_CHECKED);     /*Uncheck the current radio button*/
    *active_id = lv_obj_get_index(act_cb);

    //秒单位切换
    if (active_id = &active_index_1)//判断单选框组被选，两个单选框组同一个事件处理函数
    {
        //更新显示
        text_set();
    }
    //顺序 倒序计时切换
    else
    {
        //更新提示文本
        if (active_index_2 == 0)
        {
            lv_label_set_text(label2, "倒计时");//标签设置文本，可用printf
        }
        else
        {
            lv_label_set_text(label2, "顺计时");
        }
    }
}

//微秒计数器
int UsSleep(int us)
{
    //储存计数的联合
    LARGE_INTEGER fre;
    //获取硬件支持的高精度计数器的频率
    if (QueryPerformanceFrequency(&fre))
    {
        LARGE_INTEGER run, priv, curr, res;
        run.QuadPart = fre.QuadPart * us / 1000000;//转换为微秒级
        //获取高精度计数器数值
        QueryPerformanceCounter(&priv);
        do
        {
            QueryPerformanceCounter(&curr);
        } while (curr.QuadPart - priv.QuadPart < run.QuadPart);
        curr.QuadPart -= priv.QuadPart;
        int nres = (curr.QuadPart * 1000000 / fre.QuadPart);//实际使用微秒时间
        return nres;
    }
    return -1;
}//误差在1微秒

//计数
void time_tick(int tick)//timer控件
{
    //是否处于暂停状态
    if (pause == true)
        return;

    //倒序
    if (active_index_2 == 0)
    {
        if (count > 0)
        {
            count -= tick;//倒序
        }
        else
        {
            //计数结束
            timer_start = false;
            lv_obj_add_state(button3, LV_STATE_DISABLED);
            // 将一个或多个给定状态添加到对象。其他状态位将保持不变。如果在样式中指定,则过渡动画将从前一状态开始到当前状态
            lv_timer_pause(timer1);//使定时器挂起
            bell_on = false;
            bell_all = true;
            lv_led_on(led1);//开
        }
    }
    //顺序
    else
    {
        if (count < all_time)
        {
            count += tick;//顺序
        }
        else
        {
            //计数结束
            timer_start = false;
            lv_obj_add_state(button3, LV_STATE_DISABLED);
        }
    }
}
//线程 声音
void fun1(void* arg)
{
    while (1)
    {
        if (bell_on == true)
        {
            Beep(1000, 300);//Beep ( [频率 [, 延迟]] )播放 beep 声音(PC蜂鸣器)
        }
        Sleep(300);
        while (bell_all == true)
        {
            Beep(1000, 500);
        }
    }
}
//线程 计数器
void fun2(void* arg)//无类型指针
{
    while (1)
    {
        while (timer_start == true)
        {
            //毫秒
            if (active_index_1 == 0)
            {
                time_tick(UsSleep(1000) / 1000);
            }
            //微秒
            else
            {
                time_tick(UsSleep(1));
            }
        }
        Sleep(10);
    }
}
//报警灯闪烁
void my_timer1(lv_timer_t* timer)
{
    if (bell_on == false)
    {
        lv_led_on(led1);
        bell_on = true;
    }
    else if (bell_on == true)
    {
        lv_led_off(led1);
        bell_on = false;
    }
}
//更新数据
/*刷新数字显示
定时器工作不会刷新控件的文字
需要另一个定时器手动刷新*/
void my_timer(lv_timer_t* timer)
{
    text_set();
}
//更新数据 实时刷新界面
void text_set()
{
    //没有开始计时
    if (all_time == 0)
    {
        //毫秒
        if (active_index_1 == 0)
        {
            lv_label_set_text(label1, "0:000");//设置文本
        }
        //微秒
        else
        {
            lv_label_set_text(label1, "0:000:000");
        }
    }
    else
    {
        //毫秒
        if (active_index_1 == 0)
        {
            long temp = count % 1000;//时间换算
            long temp1 = count / 1000;
            lv_label_set_text_fmt(label1, "%d:%03d", temp1, temp);
        }
        //微秒
        else
        {
            long temp = count % 1000;
            long temp1 = count / 1000000;
            long temp2 = count / 1000 % 1000;
            /*temp = 1234567
              temp % 1000 = 567
              temp / 1000000 = 1
              temp / 1000 % 1000 = 234
              1234567 / 1000 = 12345
              1234567 % 1000 = 567
              12345 % 1000 = 345
              1234567 / 1000000 = 1*/
            //时间显示 1 234 567
            lv_label_set_text_fmt(label1, "%d:%03d:%03d", temp1, temp2, temp);
        }
        //如果是倒序 则启动倒计时报警
        if (active_index_2 == 0)
        {
            //毫秒
            if (active_index_1 == 0)
            {
                //3000/1000=3秒
                if (count < 3000 && bell_run == false) 
                {
                    bell_run = true;
                    lv_timer_resume(timer1); //使定时器进入运行状态
                }
            }
            //微秒
            else
            {
                //3000000/1000000=3秒
                if (count < 3000000 && bell_run == false)
                {
                    bell_run = true;
                    lv_timer_resume(timer1);
                }
            }
        }
    }
}
//锁定控件
void lock_ui()
{
    lv_obj_add_state(button1, LV_STATE_DISABLED);
    lv_obj_add_state(text1, LV_STATE_DISABLED);
    lv_obj_add_state(check1, LV_STATE_DISABLED);
    lv_obj_add_state(check2, LV_STATE_DISABLED);
    lv_obj_add_state(check3, LV_STATE_DISABLED);
    lv_obj_add_state(check4, LV_STATE_DISABLED);

    lv_obj_clear_state(button3, LV_STATE_DISABLED);////移除对象的一个或多个状态。
}
//解锁控件
void unlock_ui()
{
    lv_obj_clear_state(button1, LV_STATE_DISABLED);
    lv_obj_clear_state(text1, LV_STATE_DISABLED);
    lv_obj_clear_state(check1, LV_STATE_DISABLED);
    lv_obj_clear_state(check2, LV_STATE_DISABLED);
    lv_obj_clear_state(check3, LV_STATE_DISABLED);
    lv_obj_clear_state(check4, LV_STATE_DISABLED);

    lv_obj_add_state(button3, LV_STATE_DISABLED);
}
//开始计数器
void start_down()
{
    if (active_index_1 == 0)
    {
        int time = UsSleep(1);
        if (time == -1)
        {
            clear_down();
            show_message("不支持微秒计数");
            return;
        }
    }
    //倒序
    if (active_index_2 == 0)
    {
        //毫秒
        if (active_index_1 == 0)
        {
            //剩余时间=设定秒数X1000变为毫秒
            count = all_time * 1000;
        }
        //微秒
        else
        {
            //剩余时间=设定秒数X1000000
            count = all_time * 1000000;
        }
    }
    else
    {
        //毫秒
        if (active_index_1 == 0)
        {
            //目标时间=设定秒数X1000
            all_time = all_time * 1000;
        }
        else
        {
            //目标时间=设定秒数X1000000
            all_time = all_time * 1000000;
        }
    }
    //重置标志
    pause = false;
    lock_ui();
    //启动显示更新定时器
    lv_timer_resume(timer);
    //启动运行知识灯
    lv_led_on(led2);

    timer_start = true;
    //更新按键为 暂停
    lv_label_set_text(button3_label, "暂停");
}
//重置计数器 清屏
void clear_down()
{
    //重置标志
    bell_run = false;
    pause = true;
    bell_all = false;
    timer_start = false;
    //暂停定时器
    lv_timer_pause(timer);
    lv_timer_pause(timer1);
    //清理计数器值
    count = 0;
    all_time = 0;
    //更新文本
    text_set();
    //关闭所有指示灯
    lv_led_off(led2);
    lv_led_off(led1);
    unlock_ui();
}
//数字转换  
int read_time()
{
    const char* txt = lv_textarea_get_text(text1);
    //在密码模式下 lv_textarea_get_text(textarea) 给出真实文本，而不是项目符号字符
    if (strlen(txt) == 0)
        return -1;
    return atoi(txt);//将字符串转为整数
}
//弹窗提示
void show_message(char* title)
{
    lv_msgbox_t* mbox1 = (lv_msgbox_t*)lv_msgbox_create(NULL, title, NULL, NULL, true);
    lv_obj_set_style_text_font(mbox1->title, &fz24_no, 0);//字体
    lv_obj_center(mbox1);//位置中间
}
//按键处理
static void event_handler(lv_event_t* e)
{
    //按键启动按下
    if (e->target == button1)//触发按键
    {
        int time = read_time();
        if (time == -1)
        {
            show_message("没有输入时间");
            return;
        }

        all_time = time;
        start_down();
    }
    //按键重置按下
    else if (e->target == button2)
    {
        clear_down();
    }
    //按键暂停按下
    else if (e->target == button3)
    {
        if (pause == true)
        {
            pause = false;
            lv_label_set_text(button3_label, "暂停");
        }
        else
        {
            pause = true;
            lv_label_set_text(button3_label, "继续");
        }
    }
    //按键退出按下
    else if (e->target == button4)
    {
        lv_win32_quit_signal = true;//退出
    }
}
//初始化UI 美化界面 初始化控件
void init_ui()
{
    obj = lv_obj_create(lv_scr_act());//在屏幕上创造
    lv_obj_set_align(obj, LV_ALIGN_CENTER);//设置对齐
    lv_obj_set_width(obj, 500);//设置宽
    lv_obj_set_height(obj, 300);//设置高

    label2 = lv_label_create(obj);
    lv_obj_align(label2, LV_ALIGN_TOP_LEFT, 15, 5);
    //15和5代表x_ofs：对齐后的x坐标偏移
    //         y_ofs：对齐后的y坐标偏移
    lv_obj_set_style_text_font(label2, &fz24_no,0);
    //0代表显示使用多少位来描述字体中的像素
    lv_label_set_text(label2, "计时");

    label1 = lv_label_create(obj);
    lv_obj_align(label1, LV_ALIGN_TOP_LEFT, 5, 200);
    lv_obj_set_style_text_font(label1, &lv_font_montserrat_48, 0);
    lv_label_set_text(label1, "00:000");

    text1 = lv_textarea_create(obj);
    lv_textarea_set_one_line(text1, true);
    //可以将“文本”区域配置为与 的单行。 在此模式下，高度自动设置为仅显示一行，忽略换行符，并禁用自动换行。
    lv_obj_align(text1, LV_ALIGN_TOP_LEFT, 100, 0);
    lv_obj_set_width(text1, 120);
    lv_textarea_set_accepted_chars(text1, "0123456789");
    //您可以使用 设置接受字符的列表。 其他字符将被忽略
    lv_obj_add_state(text1, LV_STATE_FOCUSED);
    //LV_STATE_FOCUSED (0x02)：通过键盘或编码器聚焦或通过触摸板/鼠标单击

    lv_obj_t* label = lv_label_create(obj);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 230, 5);
    lv_obj_set_style_text_font(label, &fz24_no, 0);
    lv_label_set_text(label, "秒");

    button1 = lv_btn_create(obj);
    lv_obj_add_event_cb(button1, event_handler, LV_EVENT_CLICKED, NULL);
    //按键点击，释放后调用
    lv_obj_align(button1, LV_ALIGN_TOP_LEFT, 5, 50);
    lv_obj_set_size(button1, 80, 50);
    //w：新宽度
    //h：新高度
    label = lv_label_create(button1);
    lv_obj_set_style_text_font(label, &fz24_no, 0);
    lv_label_set_text(label, "启动");
    lv_obj_center(label);
    //将子项与其父项的中心对齐

    button2 = lv_btn_create(obj);
    lv_obj_add_event_cb(button2, event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_align(button2, LV_ALIGN_TOP_LEFT, 95, 50);
    lv_obj_set_size(button2, 80, 50);
    label = lv_label_create(button2);
    lv_obj_set_style_text_font(label, &fz24_no, 0);
    lv_label_set_text(label, "重置");
    lv_obj_center(label);

    button3 = lv_btn_create(obj);
    lv_obj_add_event_cb(button3, event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_align(button3, LV_ALIGN_TOP_LEFT, 5, 105);
    lv_obj_set_size(button3, 80, 50);
    button3_label = lv_label_create(button3);
    lv_obj_set_style_text_font(button3_label, &fz24_no, 0);
    lv_label_set_text(button3_label, "暂停");
    lv_obj_add_state(button3, LV_STATE_DISABLED);
    lv_obj_center(button3_label);

    button4 = lv_btn_create(obj);
    lv_obj_add_event_cb(button4, event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_align(button4, LV_ALIGN_TOP_LEFT, 95, 105);
    lv_obj_set_size(button4, 80, 50);
    label = lv_label_create(button4);
    lv_obj_set_style_text_font(label, &fz24_no, 0);
    lv_label_set_text(label, "退出");
    lv_obj_center(label);

    timer = lv_timer_create(my_timer, 1, NULL);
    //根据滴答速率计算实时时间-分辨率为一个滴答周期
    timer1 = lv_timer_create(my_timer1, 300, NULL);
    lv_timer_pause(timer);
    lv_timer_pause(timer1);

    led1 = lv_led_create(obj);
    lv_obj_align(led1, LV_ALIGN_TOP_RIGHT, 0, 5);
    lv_led_set_color(led1, lv_palette_main(LV_PALETTE_RED));
    lv_led_off(led1);

    led2 = lv_led_create(obj);
    lv_obj_align(led2, LV_ALIGN_TOP_RIGHT, -40, 5);
    lv_led_off(led2);

    lv_style_init(&style_radio);//样式初始化
    lv_style_set_radius(&style_radio, LV_RADIUS_CIRCLE);
    //这是用来设置控件的圆角效果的，例如按钮的四角圆角半径

    lv_style_init(&style_radio_chk);
    lv_style_set_bg_img_src(&style_radio_chk, NULL);
    //背景图像设置

    lv_obj_t* cont1 = lv_obj_create(obj);
    lv_obj_align(cont1, LV_ALIGN_TOP_LEFT, 200, 47);
    lv_obj_set_flex_flow(cont1, LV_FLEX_FLOW_ROW);
    //将孩子排成一排，不要包裹
    lv_obj_set_size(cont1, 250, 78);
    lv_obj_add_event_cb(cont1, radio_event_handler, LV_EVENT_CLICKED, &active_index_1);

    check1 = lv_checkbox_create(cont1);
    lv_checkbox_set_text(check1, "普通");
    lv_obj_add_flag(check1, LV_OBJ_FLAG_EVENT_BUBBLE);// 所有的事件也会被发送到对象的父对象
    lv_obj_add_style(check1, &style_radio, LV_PART_INDICATOR);
    //显示滑块当前状态的指示器。 还使用所有典型的背景样式属性
    lv_obj_set_style_text_font(check1, &fz24_no, 0);
    lv_obj_add_style(check1, &style_radio_chk, LV_PART_INDICATOR | LV_STATE_CHECKED);

    check2 = lv_checkbox_create(cont1);
    lv_checkbox_set_text(check2, "工业");//修改文本
    lv_obj_add_flag(check2, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(check2, &style_radio, LV_PART_INDICATOR);
    lv_obj_set_style_text_font(check2, &fz24_no, 0);
    lv_obj_add_style(check2, &style_radio_chk, LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_add_state(lv_obj_get_child(cont1, 0), LV_STATE_CHECKED);//(cont1,0)代表

    lv_obj_t* cont2 = lv_obj_create(obj);
    lv_obj_align(cont2, LV_ALIGN_TOP_LEFT, 200, 128);
    lv_obj_set_flex_flow(cont2, LV_FLEX_FLOW_ROW);
    lv_obj_set_size(cont2, 250, 78);
    lv_obj_add_event_cb(cont2, radio_event_handler, LV_EVENT_CLICKED, &active_index_2);

    check3 = lv_checkbox_create(cont2);
    lv_checkbox_set_text(check3, "倒序");
    lv_obj_add_flag(check3, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(check3, &style_radio, LV_PART_INDICATOR);
    lv_obj_set_style_text_font(check3, &fz24_no, 0);
    lv_obj_add_style(check3, &style_radio_chk, LV_PART_INDICATOR | LV_STATE_CHECKED);

    check4 = lv_checkbox_create(cont2);
    lv_checkbox_set_text(check4, "顺序");
    lv_obj_add_flag(check4, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_style(check4, &style_radio, LV_PART_INDICATOR);
    lv_obj_set_style_text_font(check4, &fz24_no, 0);
    lv_obj_add_style(check4, &style_radio_chk, LV_PART_INDICATOR | LV_STATE_CHECKED);

    /*Make the first checkbox checked*/
    lv_obj_add_state(lv_obj_get_child(cont2, 0), LV_STATE_CHECKED);
    // 从最后到第一，切换或选中
}

void start()
{
    printf("Start\n");
    init_ui();
    //报警线程
    _beginthread(fun1, 0, NULL);
    //计数线程
    _beginthread(fun2, 0, NULL);
}
