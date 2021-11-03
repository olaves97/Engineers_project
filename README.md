# Praca_Inzynierska
Program do obsługi modułu sterującego oświetleniem


W ramach pracy inżynierskiej wykonano projekt inteligentnego sterownika LED do pomieszczenia. Zaprojektowano i wykonano system bezprzewodowy do sterowania natężeniem i barwą światła. Projekt zrealizowano wykorzystując protokół komunikacyjny MQTT. System działał w oparciu o sieć domową WiFi. Do wykonania modułu wykorzystano płytkę NodeMCU v3. 
Tworzenie programu odbywało się w Arduino IDE w języku C. Broker, niezbędny do obsługi protokołu MQTT, uruchomiony został na serwerze Adafruit IO. Oświetleniem można sterować w czterech trybach (sterowanie barwami za pomocą potencjometrów, płynne przechodzenie kolorów, sterowanie natężeniem w pokoju w zależności od poziomu natężenia światła na zewnątrz, sterowanie bezprzewodowe za pomocą aplikacji).
