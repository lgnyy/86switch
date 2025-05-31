
var _client_id = "2882303761520251711";
var _redirect_url = "http://homeassistant.local:8123/api/webhook/10394765721507444209";
var _access_token = "";

var console
if (!console){
	console = {log: function(s) {Debug.writeln(s);}}
}

function gen_auth_url(redirect_url, state){
	var param = {redirect_uri:redirect_url, client_id:_client_id, response_type:"code", state:state, skip_confirm:"False"};
	var list = [];
	for(var key in param){
		list.push(key + "=" + encodeURIComponent(param[key]));
	}
	return "https://account.xiaomi.com/oauth2/authorize?" + list.join("&");
}

function get_access_token(code){
	var param = '{"client_id": ' + _client_id + ', "redirect_uri": "' + _redirect_url + '", "code": "'+code + '"}';
	var url = "https://ha.api.io.mi.com/app/v2/ha/oauth/get_token";
	url += ("?data=" + encodeURIComponent(param)); // 
	console.log(url);
	var req = new ActiveXObject("WinHttp.WinHttpRequest.5.1"); // Microsoft.XMLHTTP
	req.open("GET", url, false); 
	req.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");

	req.send('');
	console.log( req.responseText );
}

function action(data){
	var req = new ActiveXObject("WinHttp.WinHttpRequest.5.1"); // Microsoft.XMLHTTP
	req.open("POST", "https://ha.api.io.mi.com/app/v2/miotspec/action", false); 
	req.setRequestHeader("Host", "ha.api.io.mi.com");
	req.setRequestHeader("X-Client-BizId", "haapi");
	req.setRequestHeader("Content-Type", "application/json");
	req.setRequestHeader("Authorization", "Bearer" + _access_token);
	req.setRequestHeader("X-Client-AppId", "2882303761520251711");

	req.send(data);
	console.log( req.responseText );
}

// 获取access_token
if (!_access_token){
	console.log(gen_auth_url(_redirect_url, "11705502445692624711"));
	get_access_token("C3_2127D3BD78E3DF26C9656CEA5797F8D4");
}
else {
	action('{"params": {"did": "lumi.54ef441000339eca", "siid": 2, "aiid": 1, "in": []}}')
}
