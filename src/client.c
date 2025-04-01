#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>


#define DEFAULT_PORT 7000
#define INITIALS_SIZE 40

//printf("\033[XA"); // Move up X lines;
//printf("\033[XB"); // Move down X lines;
//printf("\033[XC"); // Move right X column;
//printf("\033[XD"); // Move left X column;
///printf("\033[2J"); // Clear screen

#define clear() printf("\033[H\033[J")
#define gotoxy(x,y) printf("\033[%d;%dH", (y), (x))

typedef struct {
    uv_stream_t* tcp_stream;
    char* name;              
} client_context_t;


uv_loop_t *loop;
struct sockaddr_in dest;
void read_stdin(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);


void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = (char*) malloc(suggested_size + 1);
    buf->base[suggested_size] = '\0';
    buf->len = suggested_size;
}

void on_close(uv_handle_t* handle) {
    free(handle);
}

void on_response(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf){

    if (nread > 0) {
        printf("%.*s", (int) nread, buf->base);
        free(buf->base);
        return;
    }

    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t*) stream, on_close);
    }
}

void write_handle(uv_write_t* wreq, int status){
    if (status) {
        fprintf(stderr, "uv_write error: %s\n", uv_err_name(status));
        free(wreq);
        return;
    }
    free(wreq->data);
    uv_read_start((uv_stream_t*) wreq->handle, alloc_buffer, on_response);
}   

void read_stdin(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf){
    if (nread > 0){
        buf->base[nread] = '\0';
        printf("\033[1A");
        printf("\x1b[2K");
        printf("\t\tyou: %s", buf->base);

        char* name = ((client_context_t *) stream->data)->name;
        char* tmp = calloc(strlen(name) + strlen(buf->base) + 3, sizeof(*tmp));
        sprintf(tmp, "%s: %s", name, buf->base);
 
        uv_buf_t w_buf = uv_buf_init(tmp, strlen(tmp) + 1);
        uv_write_t* wr_req = (uv_write_t* ) malloc(sizeof(uv_write_t));
        wr_req->data = tmp;

        uv_write(wr_req, (uv_stream_t*) ((client_context_t*)stream->data)->tcp_stream, &w_buf, 1, write_handle);
        
        free(buf->base);
        return;
    }
    if (nread < 0){
        if (nread != UV_EOF)
            fprintf(stderr, "Read error from stdin: %s\n", uv_err_name(nread));
        uv_close((uv_handle_t*) stream, on_close);
    }
    free(buf->base);
}

void on_connect(uv_connect_t *req, int status) {
    if (status < 0) {
        fprintf(stderr, "connect failed error %s\n", uv_err_name(status));
        free(req);
        return;
    }
    printf("Connected successfully!\n");

    uv_pipe_t* stdin_pipe = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, stdin_pipe, 0);
    uv_pipe_open(stdin_pipe, 0);

    client_context_t* con = malloc(sizeof(client_context_t));
    con->tcp_stream = req->handle;
    con->name = (char*) req->data;
    stdin_pipe->data = con;

    uv_read_start((uv_stream_t*)stdin_pipe, alloc_buffer, read_stdin);
    uv_read_start((uv_stream_t*)req->handle, alloc_buffer, on_response);
}

char* get_name(){
    char* name = calloc(INITIALS_SIZE, sizeof(*name));
    printf("Enter your name: ");
    scanf("%s", name);
    return name;
}


int main(){
    char* name = get_name();

    loop = uv_default_loop();

    uv_tcp_t* socket = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, socket);

    uv_connect_t* connect = (uv_connect_t*) malloc(sizeof(uv_connect_t));

    uv_ip4_addr("127.0.0.1", DEFAULT_PORT, &dest);

    connect->data = name;
    uv_tcp_connect(connect, socket, (const struct sockaddr*)&dest, on_connect);
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}