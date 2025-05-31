
#依赖：https://github.com/XiaoMi/ha_xiaomi_home/tree/main/custom_components/xiaomi_home/miot

import asyncio
import os
import socket
import secrets
import logging
import time
import argparse

# 用socketpair实现事件功能（win32）
class x_socketpair_eventfd:
    _event_fd: socket.socket
    _event_fd2: socket.socket
    def __init__(self, initval:int, flags: int) -> None:
        self._event_fd,self._event_fd2 = socket.socketpair()
    def write(self, n: int) -> None:
        self._event_fd2.send(b'1')
    def read(self) -> int:
        self._event_fd.recv(1)
        return 1
    def fileno(self) -> int:
        return self._event_fd.fileno()

if os.name == 'nt':  # Windows
    #asyncio.set_event_loop_policy(asyncio.WindowsProactorEventLoopPolicy())
    os.O_NONBLOCK = 0   
    os.eventfd = x_socketpair_eventfd
    os.eventfd_write = x_socketpair_eventfd.write
    os.eventfd_read = x_socketpair_eventfd.read
    
    
 
_LOGGER = logging.getLogger(__name__)

logging.basicConfig(level=logging.DEBUG,
    format='%(asctime)s.%(msecs)03d %(levelname)s %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S')


# 调用异步函数
async def main():
    parser = argparse.ArgumentParser(description='MIOT解析器选择')
    parser.add_argument('--old', action='store_true', help='使用旧版本miot_old模块')
    parser.add_argument('--token', help='授权TOKEN')
    args = parser.parse_args()

    if args.old:
        from miot_old.const import(OAUTH2_CLIENT_ID, OAUTH_REDIRECT_URL, DEFAULT_CLOUD_SERVER)
        from miot_old.miot_cloud import(MIoTOauthClient, MIoTHttpClient)
        from miot_old.miot_spec import(MIoTSpecParser)
        from miot_old.miot_mips import(MIoTDeviceState, MipsCloudClient, MipsDeviceState, MipsLocalClient)
    else:
        from miot.const import(OAUTH2_CLIENT_ID, OAUTH_REDIRECT_URL, DEFAULT_CLOUD_SERVER)
        from miot.miot_cloud import(MIoTOauthClient, MIoTHttpClient)
        from miot.miot_spec import(MIoTSpecParser)
        from miot.miot_mips import(MIoTDeviceState, MipsCloudClient, MipsDeviceState, MipsLocalClient)

    _client_id = OAUTH2_CLIENT_ID
    _redirect_url = OAUTH_REDIRECT_URL + "/api/webhook/10394765721507444209"
    _cloud_server = DEFAULT_CLOUD_SERVER
    
    _access_token = args.token
    print("_access_token:", _access_token)
    # 联机认证
    if _access_token is None :
        oauth_kwargs = {
            'client_id': _client_id,
            'redirect_url': _redirect_url,
            'cloud_server': _cloud_server,
        }

        if not args.old:
            oauth_kwargs['uuid'] = "41E8C3BE-EC3E-4210-ACB8-16FDD079C050"
    
        miot_oauth = MIoTOauthClient(**oauth_kwargs)
        state = str(secrets.randbits(64))
        _oauth_auth_url = miot_oauth.gen_auth_url(redirect_url=_redirect_url, state=state)
        _LOGGER.info('_oauth_auth_url, %s', _oauth_auth_url)

        #请在web浏览器输入以上url
        oauth_code = input("请输入单点登录后的code内容: ")
        _access_token = await miot_oauth.get_access_token_async(code=oauth_code)
        print(_access_token)

    
    miot_http = MIoTHttpClient(cloud_server=_cloud_server, client_id=_client_id, access_token=_access_token)

    #await miot_http.get_user_info_async()
    #homeinfos = await miot_http.get_homeinfos_async()
    #print("homeinfos: ", homeinfos)
    
    #devices = await miot_http.get_devices_with_dids_async(dids=['402807754', 'lumi.54ef441000339eca', '392563023'])
    devices = await miot_http.get_devices_with_dids_async(dids=['lumi.158d000755946e', 'lumi.54ef441000339eca', 'lumi.54ef441000339eca.s2', 'lumi.54ef441000339eca.s3', 'lumi.54ef441000339eca.s4'])
    print("devices: ", devices)

    
    # win32不支持add_reader；在linux运行，或用低版本：https://github.com/XiaoMi/ha_xiaomi_home/blob/v0.1.4/custom_components/xiaomi_home/miot
    if args.old or (not os.name == 'nt'):
        # 连接订阅服务
        uuid='e65ab9b3b56c758786f198972f51f199'
        _mips_cloud = MipsCloudClient(uuid=uuid, cloud_server='cn', app_id=OAUTH2_CLIENT_ID,token=_access_token)
        _mips_cloud.enable_logger(logger=_LOGGER)
        _mips_cloud.connect()
        #_device_list_gateway
        print('\nmips_cloud.connect...')
        time.sleep(2)
        
        # 订阅设备属性变动
        print('\n_mips_cloud.sub_prop...')
        def on_prop(params:dict, ctx:any) :
            print("\nparams: ", params)
        _mips_cloud.sub_prop(did="lumi.54ef441000339eca", handler=on_prop, siid=2, piid=1)
    

    # 执行动作，设置属性
    await miot_http.action_async(did="lumi.54ef441000339eca", siid=2, aiid=1, in_list=[])
    time.sleep(2)
    props = await miot_http.get_props_async(params=[{"did": "lumi.54ef441000339eca", "siid": 2, "piid": 1}])
    print("\nprops: ", props)
    #props = await miot_http.get_props_async(params=[{"did": "392563023", "siid": 2, "piid": 1}])
    #print("props: ", props)
    #prop = await miot_http.get_prop_async(did="392563023", siid=2, piid=1)
    #print("props: ", prop)
    #await miot_http.set_prop_async(params=[{"did": "392563023", "siid": 2, "piid": 1, "value": True}])
    
    # 设备特性
    if args.old :
        spec = MIoTSpecParser(lang="zh-Hans")
        await spec.init_async()
        #instance = await spec.parse(urn='urn:miot-spec-v2:device:speaker:0000A015:xiaomi-l06a:2')
        #instance = await spec.parse(urn='urn:miot-spec-v2:device:switch:0000A003:isa-kg03hl:2:0000C810')
        instance = await spec.parse(urn='urn:miot-spec-v2:device:light:0000A001:yeelink-bslamp2:2')      
        print("\ninstance.dump(): ", instance.dump())

    if not os.name == 'nt':
        time.sleep(20)

        
# 运行异步程序
if __name__ == "__main__":
    #loop = asyncio.new_event_loop()
    #asyncio.set_event_loop(loop)
    #loop.run_until_complete(main())
    asyncio.run(main()) 
