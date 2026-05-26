#include <iostream>
#include <curl/curl.h>

int main() {
    curl_version_info_data *info = curl_version_info(CURLVERSION_NOW);
    std::cout << "Hello from a secure container!" << std::endl;
    std::cout << "libcurl version: " << info->version << std::endl;
    return 0;
}
