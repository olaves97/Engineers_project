/********** MODUL STERUJACY OSWIETLENIEM **********
 *  Pracujacy w czterech trybach:
 *  1 - tryb sterowania kazda dioda osobno za pomoca potencjometrow
 *  2 - tryb dyskoteki, czyli plynnie przechodzacych kolorow
 *  3 - tryb z czujnikiem natezenia, po wybraniu koloru i zaakceptowaniu go, zaczyna dzialac czujnik, ktory dostosowuje jasnosc diod do natezenia na czujniku
 *  4 - tryb bezprzewodowy, wybieranie koloru za pomoca suwakow przez aplikacje, oraz wlaczanie i wylaczanie modulu
 */

/***************************************************
  Adafruit MQTT Library ESP8266 Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/


/************************************************ DOLACZONE BIBLIOTEKI ****************************************************/

#include <Wire.h>                   // Biblioteka do komunikacji I2C
#include <BH1750.h>                 // Biblioteka do czujnika natezenia swiatla
#include <LiquidCrystal_I2C.h>      // Biblioteka do wyswietlacza
#include <Adafruit_ADS1015.h>       // Biblioteka do przetwornika ADS1115
#include <ESP8266WiFi.h>            // Biblioteka do polaczenia ESP8266 z WiFi
#include "Adafruit_MQTT.h"          // Biblioteki sluzace do polaczenia z Brokerem za pomoca protokolu MQTT
#include "Adafruit_MQTT_Client.h"

BH1750 czujnik_swiatla;                 // Zmienna czujnika
LiquidCrystal_I2C lcd(0x27, 16, 2);     // Deklaracja wyswietlacza
Adafruit_ADS1115 ADS1115;               // Wybor uzywanego przeze mnie przetwornika - wersja 1115


/******************************************************* DEFINICJE *******************************************************/

// Definicja wyprowadzen diody
#define niebieski 15
#define zielony 16
#define czerwony 13

// Definicje przyciskow
#define przycisk1 14
#define przycisk2 12
#define przycisk3 3
#define przycisk4 1

/************************************ DANE DO POLACZENIA Z BROKEREM - Adafruit *******************************************/

#define AIO_SERVER        "io.adafruit.com"
#define AIO_SERVERPORT    1883                  
#define AIO_USERNAME      ""                    //ukryte 
#define AIO_KEY           ""                    //ukryte 

/************************************************* POLACZENIE MQTT ******************************************************/

// Stworzenie WiFiClient dla ESP8266, aby polaczyc sie z serwerem MQTT
WiFiClient client;

// Ustawienia klienta MQTT aby polaczyc sie z brokerem
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/********************************************** ZMIENNE DO TOPICOW ******************************************************/

// Ustawienie topicu, na ktory chcemy wysylac nasze dane - natezenie swiatla z czujnika
Adafruit_MQTT_Publish natezenie = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/natezenie_swiatla");

// Ustawienie topicow, na ktorych chcemy wysylac dane do modułu
Adafruit_MQTT_Subscribe przyciskOnOff = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/On_Off");
Adafruit_MQTT_Subscribe suwak_czerwony = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/kolor_czerwony");
Adafruit_MQTT_Subscribe suwak_zielony = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/kolor_zielony");
Adafruit_MQTT_Subscribe suwak_niebieski = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/kolor_niebieski");


int8_t polaczenie;                                                  // Zmienna pomocnicza do okreslenia stanu polaczenia z brokerem
uint8_t ponowna_proba = 3;                                          // Zmienna okreslajaca ilosc powtorzen przy braku polaczenia

/**********ZMIENNE DO OBSLUGI WYSWIETLACZA - laczenie z siecia, wpisywanie hasla, sprawdzenie poprawnosci polaczenia ************/

String wybor_znakow[64] = {"A","a","B","b","C","c","D","d","E","e","F","f","G","g","H","h","I","i",
                           "J","j","K","k","L","l","M","m","N","n","O","o","P","p","Q","q","R","r",
                           "S","s","T","t","U","u","V","v","W","w","X","x","Y","y","Z","z",".","-",
                           "0","1","2","3","4","5","6","7","8","9"};
                                                

int kursor=0;                             //pozycja kursora na LCD
int wybierana_SSID = 0;                   //zmienna pomocnicza do wyboru znaku z tablicy znakow przy wpisywaniu SSID
int wybierana_haslo = 0;                  //zmienna pomocnicza do wyboru znaku z tablicy znakow przy wpisywaniu hasla
String nazwa_sieci_SSID = "";             //zmienna przechowujaca nazwe sieci
String haslo_sieci = "";                  //zmienna przechowujaca haslo sieci
int sprawdzenie_polaczenia = 0;           //zmienna, ktora odpowiada za laczenie z siecia WiFi

/******************************************************************************************************************************************************************/

/*************** Funkcja do wpisywania sieci SSID ***************/

void Wpisywanie_SSID()
{
 lcd.backlight();
 lcd.print("  Wpisz SSID:");
 lcd.setCursor(kursor,1);
 lcd.print(wybor_znakow[0]);
 
 while (digitalRead(przycisk1) == HIGH)
 {
  delay(10);
  
  if(!digitalRead(przycisk2))                   //naciskamy przycisk2 aby przesuwac po tablicy znakow
  { 
    wybierana_SSID++;
    
    if(wybierana_SSID > 63)                     //wyszlismy poza rozmiar naszej tablicy znakow
    {
      wybierana_SSID = 0;
    }

    lcd.setCursor(kursor,1);
    lcd.print(wybor_znakow[wybierana_SSID]);
    while(!digitalRead(przycisk2));             //czekamy az uzytkownik pusci przycisk
    delay(200);
  }
  
  if(!digitalRead(przycisk3))                   //naciskamy przycisk3 aby przesuwac po tablicy znakow do tylu
  {
    wybierana_SSID--;
    
    if(wybierana_SSID < 0)                      //wyszlismy poza rozmiar naszej tablicy znakow
    {
      wybierana_SSID = 63;
    }
 
    lcd.setCursor(kursor,1);
    lcd.print(wybor_znakow[wybierana_SSID]);
    while(!digitalRead(przycisk3)); 
    delay(200);  
  }
  
  if(!digitalRead(przycisk4))                   //naciskamy przycisk4 aby zatwierdzic pojedynczy znak
  {
    nazwa_sieci_SSID += wybor_znakow[wybierana_SSID]; //dopisujemy znak do zmiennej odpowiedzialnej za przechowywanie informacji o nazwie naszej sieci
    kursor++;
    delay(1000);
    
    if(digitalRead(przycisk4))                  //jesli po sekundzie przycisk nie jest trzymany to dodajemy kolejny znak
    {
      wybierana_SSID = 0;
      lcd.setCursor(kursor,1);
      lcd.print(wybor_znakow[wybierana_SSID]);
    } 
  }
 }

 if (digitalRead(przycisk1) == LOW)             //naciskamy przycisk1 aby zatwierdzic caly ciag znakow, cala nazwe sieci
 {
   nazwa_sieci_SSID += wybor_znakow[wybierana_SSID]; //dopisujemy ostatni znak do nazwy sieci
   lcd.clear();
   lcd.print("Wpisana siec:");
   lcd.setCursor(0,1);
   lcd.print(nazwa_sieci_SSID);
   delay(2000);
   lcd.clear();
 }
}

/****************************************************************************************************************************************************************/

/*************** Funkcja do wpisywania hasla do sieci ***************/

void Wpisywanie_hasla_sieci()
{
 kursor = 0;
 lcd.backlight();
 lcd.clear();
 lcd.print("  Wpisz haslo:");
 lcd.setCursor(kursor,1);
 lcd.print(wybor_znakow[0]);
 
 while (digitalRead(przycisk1) == HIGH)
 {
  delay(10);
  
  if(!digitalRead(przycisk2))                   //naciskamy przycisk2 aby przesuwac po tablicy znakow
  { 
    wybierana_haslo++;
    
    if(wybierana_haslo > 63)                     //wyszlismy poza rozmiar naszej tablicy znakow
    {
      wybierana_haslo = 0;
    }

    lcd.setCursor(kursor,1);
    lcd.print(wybor_znakow[wybierana_haslo]);
    while(!digitalRead(przycisk2));             //czekamy az uzytkownik pusci przycisk
    delay(200);
  }
  
  if(!digitalRead(przycisk3))                   //naciskamy przycisk3 aby przesuwac po tablicy znakow do tylu
  {
    wybierana_haslo--;
    
    if(wybierana_haslo < 0)                      //wyszlismy poza rozmiar naszej tablicy znakow
    {
      wybierana_haslo = 63;
    }
 
    lcd.setCursor(kursor,1);
    lcd.print(wybor_znakow[wybierana_haslo]);
    while(!digitalRead(przycisk3)); 
    delay(200);  
  }
  
  if(!digitalRead(przycisk4))                   //naciskamy przycisk4 aby zatwierdzic pojedynczy znak
  {
    haslo_sieci += wybor_znakow[wybierana_haslo]; //dopisujemy znak do zmiennej odpowiedzialnej za przechowywanie informacji o nazwie naszej sieci
    kursor++;
    delay(1000);
    
    if(digitalRead(przycisk4))                  //jesli po sekundzie przycisk nie jest trzymany to dodajemy kolejny znak
    {
      wybierana_haslo = 0;
      lcd.setCursor(kursor,1);
      lcd.print(wybor_znakow[wybierana_haslo]);
    } 
  }
 }

 if (digitalRead(przycisk1) == LOW)             //naciskamy przycisk1 aby zatwierdzic caly ciag znakow, cala nazwe sieci
 {
   haslo_sieci += wybor_znakow[wybierana_haslo]; //dopisujemy ostatni znak do nazwy sieci
   lcd.clear();
   lcd.print("Wpisane haslo:");
   lcd.setCursor(0,1);
   lcd.print(haslo_sieci);
   delay(2000);
   lcd.clear();
 }
}

/****************************************************************************************************************************************************************/

void setup() {

 // Ustawienie odpowiednich stanow poczatkowych diod i przyciskow
 pinMode(niebieski, OUTPUT);    // kolor niebieski
 pinMode(zielony, OUTPUT);      // kolor zielony
 pinMode(czerwony, OUTPUT);     // kolor czerwony

 pinMode(przycisk1, INPUT_PULLUP);
 pinMode(przycisk2, INPUT_PULLUP); 
 pinMode(przycisk3, INPUT_PULLUP);
 pinMode(przycisk4, INPUT_PULLUP); 
 
 analogWrite(niebieski, LOW);                  // poczatkowo dioda wylaczona
 analogWrite(czerwony, LOW);
 analogWrite(zielony, LOW);

 // Ustawienie wyswietlacza
 lcd.begin();
 lcd.noBacklight();

 // Inicjalizacja ADS1115
 ADS1115.begin(); 

 // Aby rozlaczyc sie z poprzednio wybrana siecia
 WiFi.disconnect();

/***************************************************** CZESC ODPOWIEDZIALNA ZA POLACZENIE Z WIFI I BROKEREM *****************************************************/

 // Funkcja do wpisywania nazwy sieci
 Wpisywanie_SSID();       
                    
 // Funkcja do wpisywania hasla sieci
 Wpisywanie_hasla_sieci();   

// Uruchomienie polaczenia z WiFi
 WiFi.begin(nazwa_sieci_SSID, haslo_sieci);
 
 lcd.print("Lacze...");
 
/*************** Polaczenie z siecia WiFi ***************/

 while (WiFi.status() != WL_CONNECTED)     // Dopoki nie polaczymy sie z siecia WiFi
 {
   delay(500);
   lcd.print(".");
   sprawdzenie_polaczenia = sprawdzenie_polaczenia + 1;
   if (sprawdzenie_polaczenia > 50)
   {
    lcd.clear();
    lcd.print(" Brak polaczenia");
    lcd.setCursor(0,1);                   //ustawienie kursora na poczatku drugiej linii
    lcd.print("     z WiFi ");            
    return;
   }
 }
 
 if (WiFi.status() == WL_CONNECTED)
 {
    Wire.begin(4,5);                      //uruchomienie interfejsu I2C na pinach SDA = 4, SCL = 5, odpowiedzialnych za komunikacje z wyswietlaczem
    lcd.clear();
    lcd.backlight();                      //zalaczenie podswietlenia wyswietlacza
    lcd.setCursor(0,0);                   //ustawienie kursora na poczatku pierwszej linii
    lcd.print("   Polaczono");            //wypisanie napisu w pierwszej linii
    lcd.setCursor(0,1);                   //ustawienie kursora na poczatku drugiej linii
    lcd.print("    z WiFi");              //wypisanie napisu w drugiej linii
    delay(3000);
    lcd.noBacklight();
    lcd.clear();
 }

   // Ustawienie topicow, ktore chcemy subskrybowac
   mqtt.subscribe(&przyciskOnOff);
   mqtt.subscribe(&suwak_czerwony);
   mqtt.subscribe(&suwak_zielony);
   mqtt.subscribe(&suwak_niebieski); 
 
/*************** Polaczenie w protokole MQTT z brokerem ***************/

 while ((polaczenie = mqtt.connect()) != 0)                          // Sprawdzenie polaczenia z brokerem MQTT
 { 
   mqtt.disconnect();                                                // Rozlaczenie z MQTT
   delay(3000);                                                      // Czekamy 3 sekundy
   ponowna_proba = ponowna_proba - 1;
       
   if (ponowna_proba == 0)                                           // Jezeli po trzech probach nadal nie jestesmy polaczeni, to reset ESP8266
   {
     while (1);
     delay(1);
   }
 } 
  
 if (mqtt.connected())                                               // Jezeli polaczylo to koniec
 {
   Wire.begin(4,5);                                                  //uruchomienie interfejsu I2C na pinach SDA = 4, SCL = 5, odpowiedzialnych za komunikacje z wyswietlaczem
   lcd.clear();
   lcd.backlight();                                                 //zalaczenie podswietlenia wyswietlacza
   lcd.setCursor(0,0);                                              //ustawienie kursora na poczatku pierwszej linii
   lcd.print("   Polaczono");                                       //wypisanie napisu w pierwszej linii
   lcd.setCursor(0,1);                                              //ustawienie kursora na poczatku drugiej linii
   lcd.print("    z MQTT");                                         //wypisanie napisu w drugiej linii
   delay(3000);                                                     //opoznienie 3s
   lcd.clear();
   lcd.noBacklight();                                               //wylaczenie podswietlenia wyswietlacza
   return;
 } 
}

/************************************************************************* ZMIENNE ***********************************************************************/

// Zmienne pomocnicze do obslugi przyciskow
int x = 1;                  //zmienna do wlaczania/wylaczania modulu
int y = 1;                  //zmienna do obslugi trybu 3, aby potencjometry dzialaly poprawnie w petli az do wyboru przycisku3
int z = 1;                  //zmienna do trybu 3, aby wlaczyc czujnik
int mqtt_pomocnicza;        //zmienna do trybu 4 do wlaczania, wylaczania modulu bezprzewodowo w kazdym trybie
int mqtt_wybor_trybu_4 = 0; //zmienna do trybu 4 do wlaczania, wylaczania modulu bezprzewodowo

char wybierana = '1';       //aktualnie wybierana liczba (ktora zmieniamy przyciskami)
char obecna_wybrana = '0';  //zmienna przechowujaca informacje o wybieranym trybie

byte jasnosc1;              // jasnosc - skalowanie niebieskiego
byte jasnosc2;              // jasnosc - skalowanie zielonego
byte jasnosc3;              // jasnosc - skalowanie czerwonego

byte jasnosc_diody1;        // jasnosc zalezna od czujnika - skalowanie niebieskiego TRYB3
byte jasnosc_diody2;        // jasnosc zalezna od czujnika - skalowanie zielonego TRYB3
byte jasnosc_diody3;        // jasnosc zalezna od czujnika - skalowanie czerwonego TRYB3

int16_t wartosc1, wartosc2, wartosc3;   // zmienne 16-bitowe dla wejsc analogowych na ADS1115


/************************************************************************ FUNKCJE ************************************************************************/

/***************************************** FUNKCJA DO WYSWIETLANIA DOSTEPNYCH TRYBOW *****************************************/

void wyswietlWybierana()
{
  lcd.setCursor(0,0);
  lcd.print("  Wybierz tryb  ");
  lcd.setCursor(7,1);
  lcd.print(wybierana);  
}

/******************************************* FUNKCJA WLACZAJACA STEROWNIK ***************************************************/

void OnModulu()
{
  MQTT_connect();  
  Adafruit_MQTT_Subscribe *subskrybowanie_inf_z_adafruita;            // Zmienna do subskrybowania
   
  if (digitalRead(przycisk1) == LOW)    //jezeli przycisk1 jest wcisniety
  {
    Wire.begin(4,5);              //uruchomienie interfejsu I2C na pinach SDA = 4, SCL = 5, odpowiedzialnych za komunikacje z wyswietlaczem
    lcd.backlight();              //zalaczenie podswietlenia wyswietlacza
    lcd.setCursor(0,0);           //ustawienie kursora na poczatku pierwszej linii
    lcd.print("  Wybierz tryb");  //wypisanie napisu w pierwszej linii
    lcd.setCursor(0,1);           //ustawienie kursora na poczatku drugiej linii
    lcd.print("  (przycisk2)");   //wypisanie napisu w drugiej linii
    delay(300);                   //opoznienie 0.3s, aby od razu nie wylaczal nam sie sterownik
    x = 0;                        //wlaczenie modulu, aby pracowal w petli
    wybierana = '1';              //aby po wlaczeniu modulu, przewijanie zaczynalo sie od '1'
    mqtt_pomocnicza = 1;
    mqtt_wybor_trybu_4 = 0;
  }
}

/****************************************** FUNKCJA WYLACZAJACA STEROWNIK ***********************************************/

void OffModulu()
{
  if (digitalRead(przycisk1) == LOW)    //jezeli przycisk1 jest wcisniety
  {
    Wire.begin(4,5);    //uruchomienie interfejsu I2C na pinach SDA = 4, SCL = 5, odpowiedzialnych za komunikacje z wyswietlaczem
    lcd.clear();        //wyczyszczenie wyswietlacza
    lcd.noBacklight();  //wylaczenie podswietlenia wyswietlacza
    delay(300);         //opoznienie 0.3s, aby od razu nie wlaczal nam sie sterownik
    x = 1;              //aby wyjsc z petli i wylaczyc modul
    y = 1;              //aby wyjsc z petli, gdy modul pracowal w ktoryms trybie
    mqtt_pomocnicza == 0;
    mqtt_wybor_trybu_4 = 0;
   }
}

/**************************************** FUNKCJA ODPOWIEDZIALNA ZA WYBOR TRYBU ****************************************/

void WyborTrybu()
{
  if(digitalRead(przycisk2) == LOW)       //jezeli przycisk2 jest wcisniety - przycisk do wyboru trybu, przewijanie 1,2,3
  { 
    Wire.begin(4,5);                      //uruchomienie interfejsu I2C na pinach SDA = 4, SCL = 5, odpowiedzialnych za komunikacje z wyswietlaczem
    lcd.clear();                          //wyczyszczenie wyswietlacza
    obecna_wybrana = wybierana;           //zmienna przechowujaca inf. o trybie 1,2,3
    wyswietlWybierana();                  //funkcja wyswietlajaca, ktory tryb mozemy wybrac
    delay(500);
    wybierana++;                          //przewijanie po 1,2,3, inkrementacja zmiennej 'wybierana'
    y = 0;                                //aby wejsc do petli odpowiedzialnej za prace trybow
      
    if(wybierana > 52)                    //wyjechalismy poza cyfre 3, wracamy na poczatek czyli do 1, 51 to w kodzie ASCII cyfra 3
    {
      wybierana = '1';                    //aby znowu zaczac od pierwszej opcji, czyli 1 i przewijac
    }
    
    while(!digitalRead(przycisk2)); 
    delay(200);
  }
}

/****************** FUNKCJA ODPOWIEDZIALNA ZA ZATWIERDZENIE TRYBU I PRZEJSCIE DO ODPOWIEDNIEJ FUNKCJI *****************/

void ZatwierdzenieTrybu()               
{    
    if ((digitalRead(przycisk3) == LOW and (y == 0)) or (mqtt_wybor_trybu_4 == 1))    //jezeli przycisk3 jest wcisniety i nastapilo wczesniej wybieranie trybu
    {
      Wire.begin(4,5);                    //uruchomienie interfejsu I2C na pinach SDA = 4, SCL = 5, odpowiedzialnych za komunikacje z wyswietlaczem
      lcd.clear();                        //wyczyszczenie wyswietlacza
      wybierana = '1';                    //aby zaczac od wartosci '1'
      
      if (obecna_wybrana == '1')          //jezeli wcisniety przycisk3 na opcji1
      {      
        lcd.print("Wybrano tryb 1");      //wyswietlenie informacji o wybranym trybie w pierwszej linii
        lcd.setCursor(0,1);               //ustawienie kursora w drugiej linii
        lcd.print("Potencjometry");       //wyswietlenie opisu trybu w drugiej linii
        Tryb1();                          //uruchomienie funkcji odpowiedzialnej za tryb 1
      }
      
      else if (obecna_wybrana == '2')     //jezeli wcisniety przycisk3 na opcji2
      {   
        lcd.print("Wybrano tryb 2");      //wyswietlenie informacji o wybranym trybie w pierwszej linii
        lcd.setCursor(0,1);               //ustawienie kursora w drugiej linii
        lcd.print("Dyskoteka");           //wyswietlenie opisu trybu w drugiej linii
        Tryb2();                          //uruchomienie funkcji odpowiedzialnej za tryb 2
      }
      
      else if (obecna_wybrana == '3')    //jezeli wcisniety przycisk3 na opcji3
      { 
        lcd.print("Wybrano tryb 3");     //wyswietlenie informacji o wybranym trybie w pierwszej linii
        lcd.setCursor(0,1);              //ustawienie kursora w drugiej linii
        lcd.print("Czujnik swiatla");    //wyswietlenie opisu trybu w drugiej linii
        Tryb3();                         //uruchomienie funkcji odpowiedzialnej za tryb 3
      }
      
      else if (obecna_wybrana == '4')    //jezeli wcisniety przycisk3 na opcji4
      { 
        lcd.print("Wybrano tryb 4");     //wyswietlenie informacji o wybranym trybie w pierwszej linii
        lcd.setCursor(0,1);              //ustawienie kursora w drugiej linii
        lcd.print("Bezprzewodowy");      //wyswietlenie opisu trybu w drugiej linii
        x = 0;
        Tryb4();                         //uruchomienie funkcji odpowiedzialnej za tryb 4
      }
      
      else if (obecna_wybrana == '0')    //jezeli w trakcie korzystania z trybu zdecydujemy sie na zmiane
      {
        WyborTrybu();                    //przejscie do wyswietlacza z ponownym wyborem trybu
      }
    }
}

/*************************************** FUNKCJA DO ZMIANY KOLORU W TRYBIE 3 ***************************************/

void ZmianaKoloruTryb3()
{
  if (digitalRead(przycisk4) == LOW)    //jezeli wcisniety przycisk4 
  {
    y = 1;                              //aby wyjsc z pracy trybu3 z czujnikiem i zmienic kolor za pomoca potencjometrow
    Wire.begin(4,5);                    //uruchomienie interfejsu I2C na pinach SDA = 4, SCL = 5, odpowiedzialnych za komunikacje z wyswietlaczem
    lcd.clear();                        //wyczyszczenie wyswietlacza
    lcd.print("Zmien kolor");           //wyswietlenie napisu w pierwszej linii
    lcd.setCursor(0,1);                 //ustawienie kursora w drugiej linii
    lcd.print("Ustaw kolor i OK");      //wyswietlenie napisu w drugiej linii
    
    // Jezeli chcemy wylaczyc z poziomu aplikacji 
    MQTT_Off();
  }
}

/************************************* FUNKCJA DO POLACZENIA W PROTOKOLE MQTT ************************************/

void MQTT_connect() 
{
  int8_t polaczenie;                                                  // Zmienna pomocnicza do okreslenia stanu polaczenia
  uint8_t ponowna_proba = 3;                                          // Zmienna okreslajaca ilosc powtorzen
  
  if (mqtt.connected())                                               // Jezeli polaczylo to koniec
  {
    return;
  }

  while ((polaczenie = mqtt.connect()) != 0)                          // Sprawdzenie polaczenia z brokerem MQTT
  { 
    mqtt.disconnect();                                                // Rozlaczenie z MQTT
    delay(3000);                                                      // Czekamy 3 sekundy
    ponowna_proba = ponowna_proba - 1;
       
    if (ponowna_proba == 0)                                           // Jezeli po trzech probach nadal nie jestesmy polaczeni, to reset ESP8266
    {
      while (1);
      delay(1);
    }
  }
}

/******************************* FUNKCJA DO WLACZENIA MODULU Z POZIOMU APLIKACJI ********************************/

void MQTT_On()
 {
    MQTT_connect();                                                     // Wlaczenie funkcji do polaczenia z MQTT
  
    Adafruit_MQTT_Subscribe *subskrybowanie_inf_z_adafruita;            // Zmienna do subskrybowania 
    
    while ((subskrybowanie_inf_z_adafruita = mqtt.readSubscription(100))) 
    {
      if (subskrybowanie_inf_z_adafruita == &przyciskOnOff)            // Jezeli wcisniety przycisk On/Off
       {
        /******* Przycisk ON *******/
        if (strcmp((char *)przyciskOnOff.lastread, "OFF") == 0)        // Jezeli przycisk jest na ON, na przycisku jest napis OFF, dioda powinna sie zapalic 
        {
          
         Wire.begin(4,5);              //uruchomienie interfejsu I2C na pinach SDA = 4, SCL = 5, odpowiedzialnych za komunikacje z wyswietlaczem
         lcd.clear();     
         lcd.backlight();              //zalaczenie podswietlenia wyswietlacza
         lcd.setCursor(0,0);           //ustawienie kursora na poczatku pierwszej linii
         lcd.print("    Wlaczono");      //wypisanie napisu w pierwszej linii
         lcd.setCursor(0,1);           //ustawienie kursora na poczatku drugiej linii
         lcd.print("     modul");        //wypisanie napisu w drugiej linii
         delay(1000);                   //opoznienie 0.3s, aby od razu nie wylaczal nam sie sterownik
           
         mqtt_pomocnicza = 1;                                          // Jezeli wlaczymy modul, to mozemy sterowac suwakami, ta zmienna daje nam mozliwosc sterowania nimi po wylaczeniu, gdy jest na 1
         obecna_wybrana = '4';
         mqtt_wybor_trybu_4 = 1;
         x = 0;
         ZatwierdzenieTrybu();
        } 
      }
   }  
 }

/***************************** FUNKCJA DO WYLACZENIA MODULU Z POZIOMU APLIKACJI *********************************/

void MQTT_Off()
 {
  MQTT_connect();                                                     // Wlaczenie funkcji do polaczenia z MQTT

  Adafruit_MQTT_Subscribe *subskrybowanie_inf_z_adafruita;            // Zmienna do subskrybowania 
  
  while ((subskrybowanie_inf_z_adafruita = mqtt.readSubscription(100))) 
  {
     if (subskrybowanie_inf_z_adafruita == &przyciskOnOff)            // Jezeli wcisniety przycisk On/Off
    {
      /******* Przycisk OFF *******/
      if (strcmp((char *)przyciskOnOff.lastread, "ON") == 0)         // Jezeli przycisk jest na OFF, na przycisku jest napis ON, dioda powinna zgasnac 
      {
       digitalWrite(czerwony, LOW);                                  // Wylaczenie diod
       digitalWrite(zielony, LOW);
       digitalWrite(niebieski, LOW);
       mqtt_pomocnicza = 0;                                          // Jezeli wylaczymy modul, to nie ma mozliwosci sterowania suwakami, ta zmienna blokuje nam mozliwosc sterowania nimi po wylaczeniu, gdy jest na 0
       
       Wire.begin(4,5);               //uruchomienie interfejsu I2C na pinach SDA = 4, SCL = 5, odpowiedzialnych za komunikacje z wyswietlaczem
       lcd.clear();
       lcd.setCursor(0,0);            //ustawienie kursora na poczatku pierwszej linii
       lcd.print("   Wylaczono");     //wypisanie napisu w pierwszej linii
       lcd.setCursor(0,1);            //ustawienie kursora na poczatku drugiej linii
       lcd.print("     modul");       //wypisanie napisu w drugiej linii
       delay(1000);                   //opoznienie 0.3s, aby od razu nie wylaczal nam sie sterownik
       lcd.noBacklight();             //wylaczenie podswietlenia wyswietlacza
       lcd.clear();
       x = 1;
       y = 1;
       z = 1;
       mqtt_pomocnicza = 0;
       mqtt_wybor_trybu_4 = 0;
      }
    }
  }
 }

/************************************ FUNKCJE ODPOWIEDZIALNE ZA POSZCZEGOLNE TRYBY DZIALANIA MODULU ************************************/

/******************************** FUNKCJA DO TRYBU 1 - sterowanie kolorem za pomoca potencjometrow ********************************/

void Tryb1()
{
  while(digitalRead(przycisk2) == HIGH and digitalRead(przycisk1) != LOW and (mqtt_pomocnicza == 1))
  {
    Wire.begin(5,4);                              //uruchomienie interfejsu I2C na pinach SDA = 5, SCL = 4, odpowiedzialnych za komunikacje z przetwornikiem analogowym ADS1115
    
    // POTENCJOMETR 1 - niebieski kolor
    wartosc1 = ADS1115.readADC_SingleEnded(2);    // funkcja mierzaca napiecie miedzy wejsciem (3) a masa przetwornika, dane sa w zakresie 0-32767 dla Vcc = 6,144V, u nas zmierzone napiecie na ADS1115 wynosi okolo 3.265V
    jasnosc1 = map(wartosc1, 0, 17700, 0, 255);   // przeskalowanie wartosci z wyjscia analogowego (potencjometru) na jasnosc PWM (0-255), zakres (0-17500) wynika z zaleznosci (32767 dla 6.144V, okolo 17500 dla 3.265V)
    analogWrite(niebieski, jasnosc1);             // ustawianie jasnosci diody LED za pomoca wyjscia PWM
  
    // POTENCJOMETR 2 - zielony kolor
    wartosc2 = ADS1115.readADC_SingleEnded(1);    // funkcja mierzaca napiecie miedzy wejsciem (2) a masa przetwornika
    jasnosc2 = map(wartosc2, 0, 17700, 0, 255);   // przeskalowanie wartosci z wyjscia analogowego (potencjometru) na jasnosc PWM (0-255)
    analogWrite(zielony, jasnosc2);               // ustawianie jasnosci diody LED za pomoca wyjscia PWM
  
    // POTENCJOMETR 3 - czerwony kolor
    wartosc3 = ADS1115.readADC_SingleEnded(3);    // funkcja mierzaca napiecie miedzy wejsciem (1) a masa przetwornika
    jasnosc3 = map(wartosc3, 0, 17700, 0, 255);   // przeskalowanie wartosci z wyjscia analogowego (potencjometru) na jasnosc PWM (0-255)    
    analogWrite(czerwony, jasnosc3);              // ustawianie jasnosci diody LED za pomoca wyjscia PWM

    // Jezeli chcemy wylaczyc z poziomu aplikacji 
    MQTT_Off();
  }

    // Wylaczenie modulu  
    if (digitalRead(przycisk1)==LOW)                // jezeli przycisk1 wcisniety - chcemy wylaczyc modul, gdy tryb1 jest wlaczony
    {
      obecna_wybrana == '0';                        // zmienna przechowujaca wartosc trybu - 0 zeby wylaczyc tryb
      delay(100);
      analogWrite(niebieski, LOW);                  // wylaczenie diody
      analogWrite(czerwony, LOW);
      analogWrite(zielony, LOW);
      OffModulu();                                  // uruchomienie funkcji wylaczajacej diode
    }
  
    // Zmiana trybu 
    if (digitalRead(przycisk2)==LOW)                // jezeli przycisk2 wcisniety - chcemy zmienic tryb
    {
      obecna_wybrana == '0';                        // zmienna przechowujaca wartosc trybu - 0 zeby wylaczyc tryb, w ktorym jestesmy i przejsc do menu wyboru trybu
      analogWrite(niebieski, LOW);                  // wylaczenie diody
      analogWrite(czerwony, LOW);
      analogWrite(zielony, LOW);
    } 
}

/*********************************** FUNKCJA DO TRYBU 2 - dyskoteka, plynne przechodzenie kolorow **********************************/

void Tryb2()
{
  int zmiana = 0;                         
  int losowa_zmiana = random(255);
  
  while(digitalRead(przycisk2) == HIGH and digitalRead(przycisk1) != LOW and(mqtt_pomocnicza == 1)) 
  { 
    //Plynna zmiana kolorow 
    for (zmiana = 0; zmiana < 255; zmiana++) 
    { 
     delay(5);
     analogWrite(czerwony, losowa_zmiana);  
     analogWrite(zielony, zmiana);
     analogWrite(niebieski, 255 - zmiana);        
    } 
    
    //Plynna zmiana kolorow do tylu
    for (zmiana = 255; zmiana > 0; zmiana--) 
    { 
    delay(5);  
    analogWrite(czerwony, zmiana);  
    analogWrite(zielony, losowa_zmiana);
    analogWrite(niebieski, 255 - zmiana);   
    }

    // Jezeli chcemy wylaczyc z poziomu aplikacji 
    MQTT_Off();
  }

   // Wylaczenie modulu  
    if (digitalRead(przycisk1)==LOW)                // jezeli przycisk1 wcisniety - chcemy wylaczyc modul, gdy tryb1 jest wlaczony
    {
      obecna_wybrana == '0';                        // zmienna przechowujaca wartosc trybu - 0 zeby wylaczyc tryb
      delay(100);
      analogWrite(niebieski, LOW);                  // wylaczenie diody
      analogWrite(czerwony, LOW);
      analogWrite(zielony, LOW);
      OffModulu();                                  // uruchomienie funkcji wylaczajacej diode
    }
  
    // Zmiana trybu 
    if (digitalRead(przycisk2)==LOW)                // jezeli przycisk2 wcisniety - chcemy zmienic tryb
    {
      obecna_wybrana == '0';                        // zmienna przechowujaca wartosc trybu - 0 zeby wylaczyc tryb, w ktorym jestesmy i przejsc do menu wyboru trybu
      delay(100);
      analogWrite(niebieski, LOW);                  // wylaczenie diody
      analogWrite(czerwony, LOW);
      analogWrite(zielony, LOW);
    } 
}


/***************** FUNKCJA DO TRYBU 3 - sterowanie jasnoscia diody w zaleznosci od natezenia swiatla na czujniku **********************/
void Tryb3()
{ 
  while((y == 0) and (mqtt_pomocnicza == 1))                                     // wybor trybu 3 lub gdy chcemy zmienic kolor oswietlenia
  {
    delay(500);
    while(digitalRead(przycisk2) == HIGH and digitalRead(przycisk1) != LOW and digitalRead(przycisk3) != LOW and z == 1 and mqtt_pomocnicza == 1)
    {
      Wire.begin(5,4);                              //uruchomienie interfejsu I2C na pinach SDA = 5, SCL = 4, odpowiedzialnych za komunikacje z przetwornikiem analogowym ADS1115
      
      // POTENCJOMETR 1 - niebieski kolor
      wartosc1 = ADS1115.readADC_SingleEnded(2);    // funkcja mierzaca napiecie miedzy wejsciem (3) a masa przetwornika, dane sa w zakresie 0-32767 dla Vcc = 6,144V, u nas zmierzone napiecie na ADS1115 wynosi okolo 3.265V
      jasnosc1 = map(wartosc1, 0, 17700, 0, 255);   // przeskalowanie wartosci z wyjscia analogowego (potencjometru) na jasnosc PWM (0-255), zakres (0-17500) wynika z zaleznosci (32767 dla 6.144V, okolo 17500 dla 3.265V)
      analogWrite(niebieski, jasnosc1);             // ustawianie jasnosci diody LED za pomoca wyjscia PWM
    
      // POTENCJOMETR 2 - zielony kolor
      wartosc2 = ADS1115.readADC_SingleEnded(1);    // funkcja mierzaca napiecie miedzy wejsciem (2) a masa przetwornika
      jasnosc2 = map(wartosc2, 0, 17700, 0, 255);   // przeskalowanie wartosci z wyjscia analogowego (potencjometru) na jasnosc PWM (0-255)
      analogWrite(zielony, jasnosc2);               // ustawianie jasnosci diody LED za pomoca wyjscia PWM
    
      // POTENCJOMETR 3 - czerwony kolor
      wartosc3 = ADS1115.readADC_SingleEnded(3);    // funkcja mierzaca napiecie miedzy wejsciem (1) a masa przetwornika
      jasnosc3 = map(wartosc3, 0, 17700, 0, 255);   // przeskalowanie wartosci z wyjscia analogowego (potencjometru) na jasnosc PWM (0-255)    
      analogWrite(czerwony, jasnosc3);              // ustawianie jasnosci diody LED za pomoca wyjscia PWM

      // Jezeli chcemy wylaczyc z poziomu aplikacji 
      MQTT_Off();
    }
  
    // Wylaczenie modulu  
    if (digitalRead(przycisk1)==LOW)                // jezeli przycisk1 wcisniety - chcemy wylaczyc modul, gdy tryb1 jest wlaczony
    {
      y = 1;                                        // zmienna pozwalajaca na wyjscie z trybu 3
      obecna_wybrana == '0';                        // zmienna przechowujaca wartosc trybu - 0 zeby wylaczyc tryb
      delay(100);
      analogWrite(niebieski, LOW);                  // wylaczenie diody
      analogWrite(czerwony, LOW);
      analogWrite(zielony, LOW);
      OffModulu();                                  // uruchomienie funkcji wylaczajacej diode
    }
  
    // Zmiana trybu 
    if (digitalRead(przycisk2)==LOW)                // jezeli przycisk2 wcisniety - chcemy zmienic tryb
    {
      y = 1;                                        // zmienna pozwalajaca na wyjscie z trybu 3
      obecna_wybrana == '0';                        // zmienna przechowujaca wartosc trybu - 0 zeby wylaczyc tryb, w ktorym jestesmy i przejsc do menu wyboru trybu
      delay(100);
      analogWrite(niebieski, LOW);                  // wylaczenie diody
      analogWrite(czerwony, LOW);
      analogWrite(zielony, LOW);
    } 
  
    // Zatwierdzenie wybranego koloru i przejscie do sterowania oswietleniem w zaleznosci od natezenia swiatla na czujniku
    if (digitalRead(przycisk3) == LOW)
    {                                    
       Wire.begin(4,5);                             //uruchomienie interfejsu I2C na pinach SDA = 4, SCL = 5, odpowiedzialnych za komunikacje z wyswietlaczem
       lcd.clear();
       lcd.print("Zatwierdzono");
       lcd.setCursor(0,1);
       lcd.print("Zmiana-przycisk4");
       delay(100); 
       z = 0;                                       // zmienna pozwalajaca na wyjscie z czesci z potencjometrami i przejscie do czesci z czujnikiem
       y = 1; 
    }

    
    // Czesc z czujnikiem - jasnosc diody zmienia sie wraz z natezeniem swiatla na czujniku
    while ((z == 0) and (mqtt_pomocnicza == 1)) 
    {      
      Wire.begin(2,0);                              //uruchomienie interfejsu I2C na pinach SDA = 2, SCL = 0, odpowiedzialnych za komunikacje z czujnikiem
      czujnik_swiatla.begin();                      //inicjacja czujnika
           
      uint16_t jasnosc_na_czujniku = czujnik_swiatla.readLightLevel();    // odczytanie wartosci z czujnika
    
      jasnosc_diody1 = map(jasnosc_na_czujniku, 4000, 1, 0, jasnosc1);    // wyskalowanie jasnosci koloru z diody niebieskiej
      jasnosc_diody2 = map(jasnosc_na_czujniku, 4000, 1, 0, jasnosc2);    // wyskalowanie jasnosci koloru z diody zielonej
      jasnosc_diody3 = map(jasnosc_na_czujniku, 4000, 1, 0, jasnosc3);    // wyskalowanie jasnosci koloru z diody czerwonej
  
      // Jesli jasnosc z czujnika jest za duza (natezenie swiatla jest za wysokie) to wylaczamy diode
      while (jasnosc_na_czujniku >= 4000 and (mqtt_pomocnicza == 1)) 
      {
        digitalWrite(niebieski, LOW);    // wylaczenie diody niebieskiej
        digitalWrite(zielony, LOW);   // wylaczenie diody zielonej
        digitalWrite(czerwony, LOW);   // wylaczenie diody czerwonej       
        delay(10);
  
        // Ponowne przeliczenie wartosci natezenia na czujniku (jezeli zmalalo to mozemy znowu zapalic diode)
        uint16_t jasnosc_na_czujniku = czujnik_swiatla.readLightLevel();    // ponowne odczytanie wartosci z czujnika
          
        // Jesli jasnosc na czujniku zmniejszyla sie to ponownie ustawiamy odpowiednie jasnosci na diodzie
        if (jasnosc_na_czujniku < 4000)
        {
          jasnosc_diody1 = map(jasnosc_na_czujniku, 4000, 1, 0, jasnosc1);
          jasnosc_diody2 = map(jasnosc_na_czujniku, 4000, 1, 0, jasnosc2);
          jasnosc_diody3 = map(jasnosc_na_czujniku, 4000, 1, 0, jasnosc3);
          break;
        }
        // Jezeli chcemy wylaczyc z poziomu aplikacji 
        MQTT_Off();
        
       }
  
      // zalaczamy diode z odpowiednio wyskalowanymi czesciami skladowymi
      analogWrite(niebieski, jasnosc_diody1);
      analogWrite(zielony, jasnosc_diody2);
      analogWrite(czerwony, jasnosc_diody3);
      delay(500);

      // Jezeli chcemy wylaczyc z poziomu aplikacji 
      MQTT_Off();
      
      // Wylaczenie modulu
      if (digitalRead(przycisk1)==LOW)
      {
        obecna_wybrana == '0';
        delay(10);
        z = 1;
        analogWrite(niebieski, LOW);
        analogWrite(czerwony, LOW);
        analogWrite(zielony, LOW);
        OffModulu();
      }
    
      // Zmiana trybu 
      if (digitalRead(przycisk2)==LOW)
      {
        obecna_wybrana == '0';
        delay(10);
        z = 1;
        y = 1;
        analogWrite(niebieski, LOW);
        analogWrite(czerwony, LOW);
        analogWrite(zielony, LOW);
      } 
  
      // Jesli przycisniemy przycisk to wychodzimy ze sterowania swiatlem za pomoca czujnika i mozemy ponownie wybrac inny kolor, ktorego jasnosc ma sie zmieniac w zaleznosci od natezenia na czujniku
      if (digitalRead(przycisk4) == LOW)
      {
        y = 0;
        z = 1;
        Wire.begin(4,5);
        lcd.clear();
        lcd.print("Zmien kolor");
        lcd.setCursor(0,1);
        lcd.print("Ustaw kolor i OK");
        
        // Jezeli chcemy wylaczyc z poziomu aplikacji 
        MQTT_Off();
      }
    } 
  }
}

/***************************************** FUNKCJA DO TRYBU 4 - sterowanie bezprzewodowe *****************************************/

void Tryb4()
{
  MQTT_connect(); 
  Wire.begin(2,0);                              //uruchomienie interfejsu I2C na pinach SDA = 2, SCL = 0, odpowiedzialnych za komunikacje z czujnikiem
  czujnik_swiatla.begin();                      //inicjacja czujnika

while(digitalRead(przycisk2) == HIGH and digitalRead(przycisk1) != LOW and (mqtt_pomocnicza == 1))
{
  Wire.begin(2,0);                                                    // Ustawienie przesylu danych z czujnika
  delay(100);
  float jasnosc_na_czujniku = czujnik_swiatla.readLightLevel();       // Odczytanie wartosci z czujnika
  natezenie.publish(jasnosc_na_czujniku);                             // Przeslanie jasnosci na czujniku do brokera
  delay(2000); 

  Adafruit_MQTT_Subscribe *subskrybowanie_inf_z_adafruita;            // Zmienna do subskrybowania 

  while ((subskrybowanie_inf_z_adafruita = mqtt.readSubscription(2000))) 
  {
    if (subskrybowanie_inf_z_adafruita == &przyciskOnOff)            // Jezeli wcisniety przycisk On/Off
    {
      /******* Przycisk OFF *******/
      if (strcmp((char *)przyciskOnOff.lastread, "ON") == 0)         // Jezeli przycisk jest na OFF, na przycisku jest napis ON, dioda powinna zgasnac 
      {
       digitalWrite(czerwony, LOW);                                  // Wylaczenie diod
       digitalWrite(zielony, LOW);
       digitalWrite(niebieski, LOW);
       
       Wire.begin(4,5);              //uruchomienie interfejsu I2C na pinach SDA = 4, SCL = 5, odpowiedzialnych za komunikacje z wyswietlaczem
       lcd.clear();
       lcd.setCursor(0,0);           //ustawienie kursora na poczatku pierwszej linii
       lcd.print("    Wylaczono");     //wypisanie napisu w pierwszej linii
       lcd.setCursor(0,1);           //ustawienie kursora na poczatku drugiej linii
       lcd.print("     modul");       //wypisanie napisu w drugiej linii
       delay(2000);                   //opoznienie 0.3s, aby od razu nie wylaczal nam sie sterownik
       lcd.clear();
       lcd.noBacklight();              //zalaczenie podswietlenia wyswietlacza
       mqtt_pomocnicza = 0;           // Jezeli wylaczymy modul, to nie ma mozliwosci sterowania suwakami, ta zmienna blokuje nam mozliwosc sterowania nimi po wylaczeniu, gdy jest na 0
       x = 1;
       return;
      }
    }
    
    /******* Suwak do koloru czerwonego *******/
    if ((subskrybowanie_inf_z_adafruita == &suwak_czerwony) && (mqtt_pomocnicza == 1))                                        // Jezeli przesuniety suwak czerwony
    {
     uint16_t wartosc_na_suwaku_czerwonym = atoi((char *)suwak_czerwony.lastread);                                            // Funkcja atoi odpowiada za konwersje do int
     analogWrite(czerwony, wartosc_na_suwaku_czerwonym);                                                                      // Ustawienie wybranej wartosci na diodzie czerwonej
    } 
    
    /******* Suwak do koloru zielonego *******/
    if ((subskrybowanie_inf_z_adafruita == &suwak_zielony)&& (mqtt_pomocnicza == 1))                                          // Jezeli przesuniety suwak zielony
    {
     uint16_t wartosc_na_suwaku_zielonym = atoi((char *)suwak_zielony.lastread);  
     analogWrite(zielony, wartosc_na_suwaku_zielonym);                                                                        // Ustawienie wybranej wartosci na diodzie zielonej
    }
    
    /******* Suwak do koloru niebieskiego *******/
    if ((subskrybowanie_inf_z_adafruita == &suwak_niebieski)&& (mqtt_pomocnicza == 1))                                        // Jezeli przesuniety suwak niebieski
    {
     uint16_t wartosc_na_suwaku_niebieskim = atoi((char *)suwak_niebieski.lastread); 
     analogWrite(niebieski, wartosc_na_suwaku_niebieskim);                                                                    // Ustawienie wybranej wartosci na diodzie niebieskiej
    }
 
    // ping serwera zeby utrzymac polaczenie MQTT
    if(! mqtt.ping()) 
    {
     mqtt.disconnect();
    }
  }

 }

 // Wylaczenie modulu  
    if (digitalRead(przycisk1)==LOW)                // jezeli przycisk1 wcisniety - chcemy wylaczyc modul, gdy tryb1 jest wlaczony
    {
      obecna_wybrana == '0';                        // zmienna przechowujaca wartosc trybu - 0 zeby wylaczyc tryb
      delay(100);
      analogWrite(niebieski, LOW);                  // wylaczenie diody
      analogWrite(czerwony, LOW);
      analogWrite(zielony, LOW);
      mqtt.disconnect();
      mqtt_pomocnicza = 0;
      OffModulu();                                  // uruchomienie funkcji wylaczajacej diode
    }
  
    // Zmiana trybu 
    if (digitalRead(przycisk2)==LOW)                // jezeli przycisk2 wcisniety - chcemy zmienic tryb
    {
      obecna_wybrana == '0';                        // zmienna przechowujaca wartosc trybu - 0 zeby wylaczyc tryb, w ktorym jestesmy i przejsc do menu wyboru trybu
      delay(100);
      analogWrite(niebieski, LOW);                  // wylaczenie diody
      analogWrite(czerwony, LOW);
      analogWrite(zielony, LOW);
      mqtt.disconnect();
      mqtt_pomocnicza = 0;
    } 
}

/****************************************************************************************************************************************************************/

/****************************************************** GLOWNA PETLA PROGRAMU ******************************************************/

void loop()
{
  // Wlaczanie modulu
    OnModulu();
  
    MQTT_On();

  
  while (x == 0 && WiFi.status() == WL_CONNECTED)
  {
    delay(10); // aby nie resetowal sie nodemcu

  //Wylaczanie modulu
    OffModulu();
    MQTT_Off();
    
  //Przewijanie po mozliwosciach wyboru trybu 1-3 
    WyborTrybu();
    
  //Jezeli uzytkownik nacisnie przycisk to zatwierdza wybor trybu  
    ZatwierdzenieTrybu(); 
  }
}
