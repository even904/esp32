import requests

def check_weather_api_availability(api_key, city_code):
    # 构建API URL
    url = f"https://restapi.amap.com/v3/weather/weatherInfo?city={city_code}&key={api_key}&extensions=base"
    print(url)
    try:
        # 发起GET请求
        response = requests.get(url)

        # 检查响应状态码
        if response.status_code == 200:
            print("API is available and responded successfully.")
            # 解析并打印JSON响应内容
            data = response.json()
            print("Response Data:")
            print(data)
        else:
            print(f"Failed to reach the API. Status code: {response.status_code}")
            print("Response body:")
            print(response.text)

    except requests.exceptions.RequestException as e:
        # 处理请求异常
        print(f"An error occurred while trying to reach the API: {e}")

if __name__ == "__main__":
    # 替换为你的实际API Key和城市代码
    user_api_key = "c873fa5b2baf1a8f1b7b9db205c69ba0"
    city_code = "350322"  # 北京市东城区

    check_weather_api_availability(user_api_key, city_code)