require('config')({'ssl'})
-- load mqtt module
local mqtt = require("mqtt")
local copas = require("copas")

local host = "cn-ha.mqtt.io.mi.com"
local client_id = "ha.e65ab9b3b56c758786f198972f51f199"
local app_id = "2882303761520251711"
local access_token=arg[1] or ""
local topic_prop = "device/392563023/up/properties_changed/2/1"
--local topic_event = "device/392563023/up/event_occured/2/1"
--local topic_device_state = "device/392563023/state/#"

-- create mqtt client
local client = mqtt.client{
	uri = host .. ":8883",
	secure = true,
	id = client_id,
	username = app_id,
	password = access_token,
	clean = true,
	--ssl_module = "ssl.ssl",
	-- NOTE: copas connector
	connector = require("mqtt.luasocket-copas"), --copas Cannot specify ssl module
}
print("created MQTT client", client)

client:on{
	connect = function(connack)
		if connack.rc ~= 0 then
			print("connection to broker failed:", connack:reason_string(), connack)
			return
		end
		print("connected:", connack) -- successful connection

		-- subscribe to test topic and publish message after it
		assert(client:subscribe{ topic=topic_prop, qos=1, callback=function(suback)
			print("subscribed:", suback)
		end})
	end,

	message = function(msg)
		assert(client:acknowledge(msg))

		print("received:", msg)
		if msg.payload == 'hello' then
			print("disconnecting...")
			assert(client:disconnect())
		end
	end,

	error = function(err)
		print("MQTT client error:", err)
	end,

	close = function()
		print("MQTT conn closed")
	end
}

-- run io loop for client until connection close
copas.addthread(function()
	print("running client in separated copas thread #1...")
	mqtt.run_sync(client)

	-- NOTE: in sync mode no automatic reconnect is working, but you may just wrap "mqtt.run_sync(client)" call in a loop like this:
	-- while true do
	-- 	mqtt.run_sync(client)
	-- end
	
	print("thread #1 stopped")
end)


copas.loop()
print("done, copas loop is stopped")
