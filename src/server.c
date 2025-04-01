#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

#define DEFAULT_PORT 7000
#define DEFAULT_BACKLOG 128


typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

typedef struct client_t client_t;
struct client_t
{
    uv_tcp_t handle;
    client_t* next;
    client_t* prev; 
};


uv_loop_t *loop;
struct sockaddr_in addr;
client_t* clients_head = NULL;


void write_handle(uv_write_t *req, int status);
//void broadcast_handle(uv_write_t *req, int status);


void free_write_req(uv_write_t *req) {
    write_req_t *wr = (write_req_t*) req;
    free(wr->buf.base);
    free(wr);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = (char*) malloc(suggested_size);
    buf->len = suggested_size;
}

void on_close(uv_handle_t* handle) {
    client_t* client_ = (client_t*) handle->data;

    if (client_->prev)
        client_->prev->next = client_->next;
    
    if (client_->next)
        client_->next->prev = client_->prev;
    else
        clients_head = client_->prev;

    free(client_);
}

void broadcast_message(uv_stream_t * client, char* str, int nread){
    client_t* ptr = clients_head;
    // char prefix[] = "\t\tyou: ";
    // char* tmp;
    
    while (ptr)
    {
        if ((uv_stream_t*) &(ptr->handle) != client){
            uv_write_t* req = (uv_write_t*) malloc(sizeof(uv_write_t));
            uv_buf_t buf = uv_buf_init(str, nread + 1);
            buf.base[nread] = '\0';
            uv_write(req, (uv_stream_t*) &(ptr->handle), &buf, 1, write_handle);
        }

        ptr = ptr->prev;        
    }
    free(str);
}

void write_handle(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }
    free_write_req(req);
}

void read_handle(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        fwrite(buf->base, 1, nread, stdout);
        fflush(stdout);
        char *msg = malloc(nread + 1);
        memcpy(msg, buf->base, nread);
        msg[nread] = '\0';
        broadcast_message(client, msg, nread);
        free(buf->base);
        return;
    }
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t*) client, on_close);
    }

    free(buf->base);
}


void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        // error!
        return;
    }

    client_t* client_ = (client_t*) malloc(sizeof(client_t));
    uv_tcp_init(loop, &(client_->handle));


    client_->handle.data = client_;
    client_->prev = clients_head;
    client_->next = NULL;
    
    if (clients_head)
        clients_head->next = client_;

    clients_head = client_;


    if (uv_accept(server, (uv_stream_t*) client_) == 0) {
        uv_read_start((uv_stream_t*) client_, alloc_buffer, read_handle);
    }
    else {
        uv_close((uv_handle_t*) client_, on_close);
    }
}



int main() {
    loop = uv_default_loop();

    uv_tcp_t* server;
    uv_tcp_init(loop, server);

    uv_ip4_addr("127.0.0.1", DEFAULT_PORT, &addr);

    uv_tcp_bind(server, (const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*) server, DEFAULT_BACKLOG, on_new_connection);
    if (r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        return 1;
    }
    return uv_run(loop, UV_RUN_DEFAULT);

}