## BUG列表

- websocket没有重新连接，在出现以下错误，10秒后没有连接
```log
websocket_client: Error, no PONG received for more than 120 seconds after PING
REC_ASR: WEBSOCKET_EVENT_ERROR
websocket_client: Reconnect after 10000 ms
REC_ASR: WEBSOCKET_EVENT_DISCONNECTED
```