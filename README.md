# git-recap
detta reposetory är för alla labbar
day2

tcp och udp tester

TCP:
--
![alt text](image-4.png) wireshark
![alt text](image-1.png) terminal
^^testade med att använda fel port på klienten då misslyckades kommandot^^
--
![alt text](image-3.png)
^^testade med att inte ha servern på när man kör klient kommandot då misslyckades det också^^
----
wireshark
![alt text](image.png)
Man kan se JSON meddelandet som skickades från klienten i den fjärde columnen och man kan se JSON statusmeddelandet från servern i en sjätte columnen


UDP:
--
![alt text](image-5.png) wireshark
![alt text](image-6.png) terminal
^^testade att skicka iväg JSON meddelandet innan mottagaren var på och den verka skicka meddelandet men den säger att destinationen är unreachable^^
--
![alt text](image-8.png) wireshark
![alt text](image-7.png) terminal
^^testade att skicka iväg JSON meddelandet till en mottagare som letar efter en annan port det verkade skicka men den sa det i wireshark att destinationen är unreachable^^
----
wireshark
![alt text](image-2.png)
udp skickar JSON meddelandet i en package