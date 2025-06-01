require("config")({"socket"}) --私有：增加socket路径

--wshttpd_copas--
local io_open = io.open
local lfs = require("lfs")
local ws_server = require("websocket.server")
local copas = require("copas") -- 用于并发处理


-- MIME 类型映射
local MIME_TYPES = {
    [".html"] = "text/html",
    [".htm"] = "text/html",
    [".css"] = "text/css",
    [".js"] = "application/javascript",
    [".json"] = "application/json",
    [".png"] = "image/png",
    [".jpg"] = "image/jpeg",
    [".jpeg"] = "image/jpeg",
    [".gif"] = "image/gif",
    [".svg"] = "image/svg+xml",
    [".txt"] = "text/plain",
    [".pdf"] = "application/pdf",
}

-- 获取文件 MIME 类型
local function get_mime_type(path)
    for ext, mime_type in pairs(MIME_TYPES) do
        if path:sub(-#ext) == ext then
            return mime_type
        end
    end
    return "application/octet-stream"
end

-- 发送文件
local function send_file(client, filepath, full_path_gz)	
    local file = io_open(full_path_gz or filepath, "rb")
    if not file then return false end
    
    local content = file:read("*a")
    file:close()
    	
    local mime_type = get_mime_type(filepath)
	local header_list = {
		"HTTP/1.1 200 OK",
		"Content-Type: " .. mime_type,
		string.format("Content-Length: %d", #content),
		"Connection: close"
	}
	if full_path_gz then
		table.insert(header_list, "Content-Encoding: gzip")
	end
	table.insert(header_list, "\r\n")
	local header = table.concat(header_list, "\r\n")
    
    client:send(header)
	client:send(content)
    return true
end

-- 发送目录列表
local function send_dir_listing(client, path)
    local files = {}
    for file in lfs.dir(path) do
        if file ~= "." and file ~= ".." then
            table.insert(files, file)
        end
    end
    
    table.sort(files)
    
    local html1 = [[
<!DOCTYPE html>
<html>
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
    <title>Index of ]] .. path .. [[</title>
    <style>
        body { font-family: sans-serif; }
        a { color: #0366d6; text-decoration: none; }
        a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <h1>Index of ]] .. path .. [[</h1>
    <hr>
    <ul>
]]
    
	local html2_list = {}
    for _, file in ipairs(files) do
        local full_path = path .. "/" .. file
        local attr = lfs.attributes(full_path)
        if attr.mode == "directory" then
            file = file .. "/"
        end
        table.insert(html2_list, string.format('<li><a href="%s">%s</a></li>\n', file, file))
    end
    local html2 = table.concat(html2_list)
    
    local html3 = [[
    </ul>
    <hr>
</body>
</html>
]]
	
    local header = table.concat({
        "HTTP/1.1 200 OK",
        "Content-Type: text/html",
        string.format("Content-Length: %d",  #html1 + #html2 + #html3), 
		"Connection: close",
        "\r\n"
    }, "\r\n")
    
    client:send(header)
    client:send(html1)
    client:send(html2)
    client:send(html3)
end

	
-- HTTP/WebSocket 请求分发
local function handle_request(client, headers, root_dir)
	local request = headers[1]
    
    -- 解析出path
    local path = request:match("^[A-Z]+%s+(.-)%s+HTTP/%d%.%d$")
    
    -- 处理普通 HTTP 请求
    if path then
		local pos = path:find("?")
		if pos then
			path = path:sub(1, pos-1)
		end

        -- URL 解码
        path = path:gsub("%%(%x%x)", function(h) return string.char(tonumber(h, 16)) end)
        -- 安全处理
        path = path:gsub("/%.%./", "/"):gsub("/%.$", ""):gsub("/$", "")

        local full_path = root_dir .. path
        
        -- 检查文件是否存在
        local attr = lfs.attributes(full_path)
        if not attr then
			local full_path_gz = full_path .. ".gz"
			if not lfs.attributes(full_path_gz) then
				client:send("HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found")
			elseif not send_file(client, full_path, full_path_gz) then
                client:send("HTTP/1.1 500 Internal Server Error\r\n\r\n")
			end
        elseif attr.mode == "directory" then
            send_dir_listing(client, full_path)
        else
            if not send_file(client, full_path) then
                client:send("HTTP/1.1 500 Internal Server Error\r\n\r\n")
            end
        end
    end
    
    client:close()
end

-- 启动服务器
function start(cfg)
	local root_dir = cfg.root_dir or lfs.currentdir()
	print("HTTP + WebSocket server running on port " .. cfg.port)
	print("Static files served from: " .. root_dir)
	print("WebSocket endpoint: ws://" .. (cfg.host or "*") .. ":" .. cfg.port .. (cfg.ws_path or "/ws"))

	local server = ws_server.copas.listen({
		ssl_params = cfg.ssl_params,
		interface = cfg.host,
		port = cfg.port,
		on_request = function(sock, headers)
			local client = copas.wrap(sock)
			if (not cfg.on_request) or (not cfg.on_request(client, headers)) then
				handle_request(client, headers, root_dir)
			end
		end,
		default = cfg.default
	})
	
	copas.loop()
end
--wshttpd_copas--



local function on_request(client, headers)
	local request = headers[1]
    
    -- 解析出path
    local path = request:match("^[A-Z]+%s+(.-)%s+HTTP/%d%.%d$")
	print(path)
	
	local function get_content(client, headers)
		for _,v in ipairs(headers) do
			local length = v:lower():match("content%-length:%s*(%d+)")
			if length then
				return client:receive(tonumber(length))
			end
		end
	end

	local function send_response(client, html, ctype)
		local header = table.concat({
			"HTTP/1.1 200 OK",
			"Content-Type: " .. (ctype or "text/html"),
			string.format("Content-Length: %d",  #html), 
			"Connection: close",
			"\r\n"
		}, "\r\n")    
		client:send(header)
		client:send(html)
		return true
	end
	
	if path:find("^/api/get_expires_ts") then
		return send_response(client, "1748856589")
	elseif path:find("^/api/gen_auth_code") then
		local html = "https://account.xiaomi.com/oauth2/authorize?redirect_uri=http%3A%2F%2Fhomeassistant.local%3A8123%2Fapi%2Fwebhook%2F10394765721507444209&client_id=2882303761520251711&response_type=code&state=11914195592848411575&skip_confirm=False&source_ip=192.168.3.44"
		return send_response(client, html)
	elseif path:find("^/api/webhook") then
		return send_response(client, "ok")
	elseif path:find("^/api/get_qweather_config") then
		return send_response(client, "116.39145,39.9073;c98d7cb82cdc49b7a20f8367e865868a");
	elseif path:find("^/api/put_qweather_config") then
		local body = get_content(client, headers)
		print("body:", body)
		return send_response(client, "ok");
	elseif path:find("^/api/get_devices_config") then
		return send_response(client, 'aaa,1,2;bbb,1,2;ccc,1,2;ddd,1,2;eee,1,2,3,4,5,5');
	elseif path:find("^/api/miot_cloud/get_homeinfos") then
		local body = get_content(client, headers)
		print("body:", body)
		local rboby = '{"code":0,"message":"","result":{"homelist":[{"id":"111111111111","name":"默认家庭","roomlist":[{"id":"111111222222","name":"卧室","bssid":"","parentid":"111111111111","dids":["aaa","bbb","ccc","ddd","eee"]},{"id":"111111333333","name":"卫生间","bssid":"","parentid":"111111111111","dids":["ffff","ggg"]}]}]}}'
		return send_response(client, rboby, "application/json");
	elseif path:find("^/api/miot_cloud/get_devices") then
		local body = get_content(client, headers)
		print("body:", body)
		local rboby
		if body:find("aaa") then
			rboby = '{"code":0,"message":"","result":{"list":[{"did":"aaa","name":"小爱音箱","spec_type":"urn:miot-spec-v2:device:speaker:0000A015:xiaomi-l06a:2"},{"did":"bbb","name":"Aqara智能墙壁开关 D1（单火线三键版）","spec_type":"urn:miot-spec-v2:device:switch:0000A003:lumi-l3acn3:1"},{"did":"eee","name":"台灯","spec_type":"urn:miot-spec-v2:device:light:0000A001:yeelink-bslamp2:2"}]}}'
		else
			rboby = '{"code":0,"message":"","result":{"list":[{"did":"fff","name":"小爱音箱2","spec_type":"urn:miot-spec-v2:device:speaker:0000A015:xiaomi-l06a:2"}]}}'
		end
		return send_response(client, rboby, "application/json");
	end	
end

local function handle_default(ws)
	while true do
		local msg,opcode = ws:receive()
		if not msg then
			ws:close()
			return
		end
		print('[S]receive',msg,opcode)
		ws:send(msg .. msg)
	end
end


start({port=8123, root_dir="E:/git/lgnyy/86switch/main", on_request=on_request, default=handle_default})
