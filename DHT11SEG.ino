#include <Wire.h>
#include "TM1650.h"
#include <ESP8266WiFi.h>
#include <dht.h>
#include <BH1750FVI.h>

#define DHT11_PIN D2
#define BUTTON_1 0x44
#define BUTTON_2 0x45
#define BUTTON_3 0x46
#define BUTTON_4 0x47

const char* ssid     = "NULL";//wifi的名称
const char* password = "123546987";//wifi的密码
const char* host ="api.heclouds.com";//访问的网址
const int httpPort = 80;//http的端口号

// Create the Lightsensor instance
BH1750FVI LightSensor(BH1750FVI::k_DevModeContLowRes);
WiFiClient client;
dht DHT;
TM1650 d;

char line[] = "0000";
float TEMP,HUM;
byte addr[8];
byte data[12];

//0温度 1湿度 2光强
byte showWihch = 0;



/*扫描并读取tm1650中的按键码 */
char buttonReading(void) {
	char buttonCode=0x00;
	Wire.beginTransmission(0x00);      //确认从机的地址开始传输
	Wire.write(0x49);                  //发送读取键盘码的指令
	Wire.requestFrom(0x24,1,0);       //主机请求数据
	buttonCode=Wire.read();
	Wire.endTransmission();          //结束传输
	return buttonCode;
}

/*根据获得的扫描码来确定生效的按键*/
void keyJudge(char buttoncode,char line[4]) {
	switch(buttoncode) {
	case BUTTON_4:
		Serial.println("k4");
		showWihch = (++showWihch) % 3;
		break;
	case BUTTON_3:
		Serial.println("k3");
		showWihch = 2;
		break;
	case BUTTON_2:
		Serial.println("k2");
		showWihch = 1;
		break;
	case BUTTON_1:
		Serial.println("k1");
		showWihch = 0;
		break;
	default:
		Serial.println("nothing");
		break;
	}
}

void setup() {
	Wire.begin();
	Serial.begin(9600);
	LightSensor.begin();
	d.init();                                       //初始化
	d.displayOff();
	d.setBrightness(TM1650_MAX_BRIGHT);             //设定亮度
	d.displayOn();
	delay(2000);
	d.displayString(line);
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
		d.displayString("----");
	}

	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
}

void loop() {

	keyJudge(buttonReading(),line);

	uint16_t lux = LightSensor.GetLightIntensity();
	Serial.print("Light: ");
	Serial.println(lux);

	int chk = DHT.read11(DHT11_PIN);
	switch (chk) {
	case DHTLIB_OK:
		Serial.print("OK,\t");
		break;
	case DHTLIB_ERROR_CHECKSUM:
		Serial.print("Checksum error,\t");
		break;
	case DHTLIB_ERROR_TIMEOUT:
		Serial.print("Time out error,\t");
		break;
	default:
		Serial.print("Unknown error,\t");
		break;
	}
	Serial.print(DHT.humidity, 1);
	Serial.print(",\t");
	Serial.println(DHT.temperature, 1);

	pinMode(DHT11_PIN, OUTPUT);
	digitalWrite(DHT11_PIN, LOW);

	TEMP = DHT.temperature;
	HUM = DHT.humidity;
	char temp[]="";

	//控制显示的项目
	switch(showWihch) {
	case 0:
		dtostrf(TEMP,4,0,temp);
		break;
	case 1:
		dtostrf(HUM,4,0,temp);
		break;
	case 2:
		dtostrf(lux,4,0,temp);
		break;
	}
	d.displayString(temp);

	String postdata = String("")+"{\"temperature\":"+TEMP+",\"humidity\":"+HUM+",\"light\":"+lux+"}";

	WiFiClient client;

	if (!client.connect(host, httpPort)) {
		Serial.println("connection failed");
		return;
	}
	String url = "/devices/571576057/datapoints?type=3";
	Serial.print("Requesting URL: ");
	Serial.println(url);

	// This will send the request to the server
	client.print(String("POST ") + url + " HTTP/1.1\r\n" +
	             "api-key:iLdZeUIr8qCMsA=BPyHfQckQdrM=\r\n"+
	             "Host:" + host + "\r\n" +
	             "Content-Length:"+postdata.length()+"\r\n\r\n"+
	             //"Content-Type: application/x-www-form-urlencoded\r\n\r\n"+
	             postdata+"\r\n");

	unsigned long timeout = millis();

	while (client.available() == 0) {
		if (millis() - timeout > 5000) {
			Serial.println(">>> Client Timeout !");
			client.stop();
			return;
		}
	}

	// Read all the lines of the reply from server and print them to Serial
	while(client.available()) {
		String line = client.readStringUntil('\r');
		Serial.print(line);
	}

	Serial.println();
	Serial.println("closing connection");
}
