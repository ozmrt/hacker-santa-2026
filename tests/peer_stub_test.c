#undef NDEBUG
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "core/esp_comm_manager.h"
int main(void) {
    esp_comm_manager_init_with_defaults(); esp_comm_manager_init(6,7,115200);
    assert(!esp_comm_manager_set_pins(6,7));
    assert(!esp_comm_manager_start_discovery());
    assert(!esp_comm_manager_connect_to_peer("peer"));
    assert(!esp_comm_manager_send_command("test",""));
    assert(!esp_comm_manager_send_command_line("test"));
    assert(!esp_comm_manager_is_connected()); assert(esp_comm_manager_get_state()==COMM_STATE_IDLE);
    esp_comm_manager_set_command_callback(NULL,NULL);esp_comm_manager_set_response_callback(NULL,NULL);
    esp_comm_manager_set_data_callback(NULL,NULL);esp_comm_manager_set_command_end_callback(NULL,NULL);
    esp_comm_manager_set_remote_command_flag(true);
    assert(!esp_comm_manager_is_remote_command());assert(!esp_comm_manager_should_forward_output());
    assert(!esp_comm_manager_send_response(NULL,0));assert(!esp_comm_manager_send_stream(0,NULL,0));
    assert(!esp_comm_manager_send_stream_wait(0,NULL,0,100));
    assert(!esp_comm_manager_register_stream_handler(0,NULL,NULL));
    char name[8]="filled";assert(!esp_comm_manager_get_peer_name(name,sizeof(name)));assert(!name[0]);
    assert(!esp_comm_manager_get_peer_name(NULL,0));
    gpio_num_t tx=6,rx=7;assert(!esp_comm_manager_get_pins(&tx,&rx));assert(tx==-1 && rx==-1);
    esp_comm_manager_stats_t stats,zero={0};memset(&stats,0xa5,sizeof(stats));
    assert(!esp_comm_manager_get_stats(&stats));assert(!memcmp(&stats,&zero,sizeof(stats)));
    assert(!esp_comm_manager_get_stats(NULL));assert(!esp_comm_manager_get_pins(NULL,NULL));
    assert(esp_comm_manager_get_baud()==0);esp_comm_manager_disconnect();esp_comm_manager_deinit();
    puts("Peer stubs: every API rejects hardware use and returns empty state");
}
