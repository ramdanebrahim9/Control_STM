#include "Control.hpp"
#include "buffer_handler.hpp"

void control_loop_step(void)
{
    const float u_k = 0.0f;
    const float Va_k = 1.0f;
    const float Vb_k = 2.0f;
    const float current_k = 3.0f;
    const int32_t ticks_k = 4;

    Sample_Data data_k = {};
    data_k.u = u_k;
    data_k.Va = Va_k;
    data_k.Vb = Vb_k;
    data_k.current = current_k;
    data_k.ticks = ticks_k;

    BUFFER_HANDLER::FillBuffer(data_k);
}