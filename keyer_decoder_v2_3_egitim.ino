
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);


// ======================================================
// PINLER
// ======================================================

#define DOT_PIN       2
#define DASH_PIN      3
#define SPEAKER_PIN   12

#define WPM_UP_PIN    4
#define WPM_DOWN_PIN  5

#define TRAIN_PIN     6       // Eğitim modu butonu


// ======================================================
// WPM
// ======================================================

int wpm = 25;

int dotTime;
int dashTime;
int elementGap;
int letterGap;
int wordGap;


// ======================================================
// MODE
// ======================================================

bool iambicMode = true;
bool trainingMode = false;


// ======================================================
// STATE
// ======================================================

bool keying = false;
bool lastWasDot = false;
bool squeeze = false;

String currentSymbol = "";
unsigned long lastActivity = 0;


// ======================================================
// LCD CURSOR TAKİBİ
// ======================================================

int cursorCol = 0;
int cursorRow = 0;


// ======================================================
// WORD GAP FLAG
// ======================================================

bool wordPrinted = false;


// ======================================================
// MORSE TABLOSU
// ======================================================

struct MorseMap {
  const char* code;
  char letter;
};

MorseMap morseTable[] = {

  {".-", 'A'},
  {"-...", 'B'},
  {"-.-.", 'C'},
  {"-..", 'D'},
  {".", 'E'},
  {"..-.", 'F'},
  {"--.", 'G'},
  {"....", 'H'},
  {"..", 'I'},
  {".---", 'J'},
  {"-.-", 'K'},
  {".-..", 'L'},
  {"--", 'M'},
  {"-.", 'N'},
  {"---", 'O'},
  {".--.", 'P'},
  {"--.-", 'Q'},
  {".-.", 'R'},
  {"...", 'S'},
  {"-", 'T'},
  {"..-", 'U'},
  {"...-", 'V'},
  {".--", 'W'},
  {"-..-", 'X'},
  {"-.--", 'Y'},
  {"--..", 'Z'},

  {"-----",'0'},
  {".----",'1'},
  {"..---",'2'},
  {"...--",'3'},
  {"....-",'4'},
  {".....",'5'},
  {"-....",'6'},
  {"--...",'7'},
  {"---..",'8'},
  {"----.",'9'}
};


// ======================================================
// EĞİTİM DEĞİŞKENLERİ
// ======================================================

char trainingTarget = '?';

String trainingUserCode = "";

bool trainingWaiting = false;

unsigned long trainingLastActivity = 0;


// ======================================================
// LCD SATIRLARI
// ======================================================

String lcdLines[4] = {
  "",
  "",
  "",
  ""
};


// ======================================================
// TIMING
// ======================================================

void updateTiming() {

  dotTime = 1200 / wpm;

  dashTime = dotTime * 3;

  elementGap = dotTime;

  letterGap = dotTime * 2;

  wordGap = dotTime * 7;
}


// ======================================================
// INPUT
// ======================================================

bool dotPressed() {

  return digitalRead(DOT_PIN) == LOW;
}


bool dashPressed() {

  return digitalRead(DASH_PIN) == LOW;
}


// ======================================================
// SES
// ======================================================

void toneOn() {

  tone(SPEAKER_PIN, 700);
}


void toneOff() {

  noTone(SPEAKER_PIN);
}


// ======================================================
// LCD'Yİ YENİLE
// ======================================================

void refreshLCD() {

  for (int row = 0; row < 4; row++) {

    lcd.setCursor(0, row);

    String line = lcdLines[row];

    while (line.length() < 20) {

      line += " ";
    }

    if (line.length() > 20) {

      line = line.substring(0, 20);
    }

    lcd.print(line);
  }
}


// ======================================================
// NORMAL MODDA LCD'YE KARAKTER YAZ
// ======================================================

void lcdPrintChar(char c) {

  // 4. satır dolmuşsa
  if (cursorCol >= 20) {

    // Satırları yukarı kaydır
    lcdLines[0] = lcdLines[1];
    lcdLines[1] = lcdLines[2];
    lcdLines[2] = lcdLines[3];

    // En alt satırı boşalt
    lcdLines[3] = "";

    cursorRow = 3;
    cursorCol = 0;

    refreshLCD();
  }


  lcdLines[cursorRow] += c;

  lcd.setCursor(cursorCol, cursorRow);
  lcd.print(c);

  cursorCol++;


  if (cursorCol >= 20) {

    if (cursorRow < 3) {

      cursorRow++;
      cursorCol = 0;
    }
    else {

      cursorCol = 20;
    }
  }
}


// ======================================================
// NORMAL MORSE GÖNDERİMİ
// ======================================================

void sendElement(bool isDot) {

  keying = true;

  toneOn();

  if (isDot)
    delay(dotTime);
  else
    delay(dashTime);

  toneOff();

  delay(elementGap);


  if (isDot) {

    currentSymbol += ".";

    lastWasDot = true;
  }
  else {

    currentSymbol += "-";

    lastWasDot = false;
  }


  lastActivity = millis();

  wordPrinted = false;

  keying = false;
}


// ======================================================
// MORSE ÇÖZÜCÜ
// ======================================================

char decodeMorse(String code) {

  for (int i = 0; i < sizeof(morseTable) / sizeof(MorseMap); i++) {

    if (code == morseTable[i].code)
      return morseTable[i].letter;
  }

  return '?';
}


// ======================================================
// NORMAL DECODER
// ======================================================

void handleDecoder() {

  if (currentSymbol.length() > 0) {

    if (millis() - lastActivity > letterGap) {

      char c = decodeMorse(currentSymbol);

      lcdPrintChar(c);

      currentSymbol = "";

      wordPrinted = false;
    }
  }


  if ((millis() - lastActivity > wordGap) && !wordPrinted) {

    lcdPrintChar(' ');

    wordPrinted = true;
  }
}


// ======================================================
// KEYER
// ======================================================

void handleKeyer() {

  bool dot = dotPressed();
  bool dash = dashPressed();


  if (!iambicMode) {

    if (dot)
      toneOn();
    else
      toneOff();

    return;
  }


  squeeze = dot && dash;


  if (!keying) {

    if (squeeze) {

      if (lastWasDot)
        sendElement(false);
      else
        sendElement(true);
    }

    else if (dot) {

      sendElement(true);
    }

    else if (dash) {

      sendElement(false);
    }
  }
}


// ======================================================
// EĞİTİM EKRANINI GÖSTER
// ======================================================

void showTrainingScreen() {

  lcdLines[0] = "   EGITIM MODU";
  lcdLines[1] = "Hedef: ";
  lcdLines[1] += trainingTarget;

  lcdLines[2] = "Senin kodun: ";

  lcdLines[2] += trainingUserCode;

  lcdLines[3] = "WPM: ";
  lcdLines[3] += wpm;

  refreshLCD();
}


// ======================================================
// RASTGELE EĞİTİM KARAKTERİ
// ======================================================

char getRandomTrainingCharacter() {

  int n = random(0, 36);

  if (n < 26) {

    return 'A' + n;
  }

  else {

    return '0' + (n - 26);
  }
}


// ======================================================
// EĞİTİM KARAKTERİNİN MORSE KODUNU BUL
// ======================================================

String getMorseCode(char c) {

  for (int i = 0; i < sizeof(morseTable) / sizeof(MorseMap); i++) {

    if (morseTable[i].letter == c) {

      return String(morseTable[i].code);
    }
  }

  return "";
}


// ======================================================
// EĞİTİM KARAKTERİNİ SESLENDİR
// ======================================================

void playTrainingMorse() {

  String code = getMorseCode(trainingTarget);


  for (int i = 0; i < code.length(); i++) {

    if (code[i] == '.') {

      toneOn();

      delay(dotTime);

      toneOff();
    }

    else if (code[i] == '-') {

      toneOn();

      delay(dashTime);

      toneOff();
    }


    // Elemanlar arası boşluk
    if (i < code.length() - 1) {

      delay(elementGap);
    }
  }


  // Cihazın göndermesi bittikten sonra
  // kullanıcının cevap vermesi için bekleme
  delay(dotTime * 2);
}


// ======================================================
// YENİ EĞİTİM SORUSU
// ======================================================

void newTrainingQuestion() {

  trainingTarget = getRandomTrainingCharacter();

  trainingUserCode = "";

  currentSymbol = "";

  trainingWaiting = true;

  showTrainingScreen();

  // Hedef karakterin Morse kodunu seslendir
  playTrainingMorse();

  // Kullanıcının cevabını bekle
  trainingLastActivity = millis();
}


// ======================================================
// EĞİTİM MODUNA GİR
// ======================================================

void enterTrainingMode() {

  trainingMode = true;

  currentSymbol = "";

  trainingUserCode = "";

  cursorCol = 0;
  cursorRow = 0;

  lcd.clear();

  delay(100);

  newTrainingQuestion();
}


// ======================================================
// EĞİTİM MODUNDAN ÇIK
// ======================================================

void exitTrainingMode() {

  trainingMode = false;

  trainingUserCode = "";

  currentSymbol = "";

  cursorCol = 0;
  cursorRow = 0;

  lcdLines[0] = "";
  lcdLines[1] = "";
  lcdLines[2] = "";
  lcdLines[3] = "";

  lcd.clear();
}


// ======================================================
// EĞİTİMDE DOĞRU CEVAP
// ======================================================

void trainingCorrect() {

  lcdLines[0] = "   DOGRU!";
  lcdLines[1] = "Hedef: ";
  lcdLines[1] += trainingTarget;

  lcdLines[2] = "Kod: ";
  lcdLines[2] += trainingUserCode;

  lcdLines[3] = "Yeni soru...";

  refreshLCD();

  delay(1000);

  if (trainingMode) {

    newTrainingQuestion();
  }
}


// ======================================================
// EĞİTİMDE YANLIŞ CEVAP
// ======================================================

void trainingWrong(char decoded) {

  lcdLines[0] = "   YANLIS!";

  lcdLines[1] = "Hedef: ";
  lcdLines[1] += trainingTarget;

  lcdLines[2] = "Sen: ";
  lcdLines[2] += decoded;

  lcdLines[3] = "Tekrar dene";

  refreshLCD();

  delay(900);

  if (trainingMode) {

    trainingUserCode = "";

    showTrainingScreen();
  }
}


// ======================================================
// EĞİTİM DECODER
// ======================================================

void handleTrainingDecoder() {

  // Kullanıcı bir veya daha fazla element girdiyse
  if (currentSymbol.length() > 0) {

    // Harf aralığı oluştuysa
    if (millis() - lastActivity > letterGap) {

      char decoded = decodeMorse(currentSymbol);

      trainingUserCode = currentSymbol;

      currentSymbol = "";


      // Doğru cevap
      if (decoded == trainingTarget) {

        trainingCorrect();
      }

      // Yanlış cevap
      else {

        trainingWrong(decoded);
      }
    }
  }
}


// ======================================================
// EĞİTİM KEYER
// ======================================================

void handleTrainingKeyer() {

  bool dot = dotPressed();
  bool dash = dashPressed();


  squeeze = dot && dash;


  if (!keying) {

    if (squeeze) {

      if (lastWasDot)
        sendElement(false);
      else
        sendElement(true);
    }

    else if (dot) {

      sendElement(true);
    }

    else if (dash) {

      sendElement(false);
    }
  }
}


// ======================================================
// EĞİTİM MODU
// ======================================================

void handleTrainingMode() {

  handleTrainingKeyer();

  handleTrainingDecoder();
}


// ======================================================
// BUTONLAR
// ======================================================

void handleButtons() {

  static unsigned long lastDebounce = 0;

  if (millis() - lastDebounce < 200)
    return;


  // ====================================================
  // EĞİTİM BUTONU D6
  // ====================================================

  if (!digitalRead(TRAIN_PIN)) {

    if (!trainingMode) {

      enterTrainingMode();
    }

    else {

      exitTrainingMode();
    }

    lastDebounce = millis();

    return;
  }


  // ====================================================
  // WPM ARTIR
  // ====================================================

  if (!digitalRead(WPM_UP_PIN)) {

    wpm++;

    if (wpm > 40)
      wpm = 40;

    updateTiming();


    if (trainingMode) {

      showTrainingScreen();
    }

    else {

      lcd.setCursor(0, 3);

      lcd.print("WPM: ");
      lcd.print(wpm);
      lcd.print("   ");
    }


    lastDebounce = millis();
  }


  // ====================================================
  // WPM AZALT
  // ====================================================

  if (!digitalRead(WPM_DOWN_PIN)) {

    wpm--;

    if (wpm < 5)
      wpm = 5;

    updateTiming();


    if (trainingMode) {

      showTrainingScreen();
    }

    else {

      lcd.setCursor(0, 3);

      lcd.print("WPM: ");
      lcd.print(wpm);
      lcd.print("   ");
    }


    lastDebounce = millis();
  }
}


// ======================================================
// SETUP
// ======================================================

void setup() {

  pinMode(DOT_PIN, INPUT_PULLUP);
  pinMode(DASH_PIN, INPUT_PULLUP);

  pinMode(SPEAKER_PIN, OUTPUT);

  pinMode(WPM_UP_PIN, INPUT_PULLUP);
  pinMode(WPM_DOWN_PIN, INPUT_PULLUP);

  pinMode(TRAIN_PIN, INPUT_PULLUP);


  lcd.init();
  lcd.backlight();


  updateTiming();


  // Rastgele sayı başlangıcı
  randomSeed(analogRead(A0));


  // Açılış ekranı

  lcd.setCursor(0, 0);
  lcd.print("   TA3OD");

  lcd.setCursor(0, 1);
  lcd.print("Cift Kol Maniple");

  lcd.setCursor(0, 2);
  lcd.print("iambic Key");

  lcd.setCursor(0, 3);
  lcd.print("WPM: ");
  lcd.print(wpm);


  delay(3000);


  lcd.clear();


  // LCD hafızasını temizle

  lcdLines[0] = "";
  lcdLines[1] = "";
  lcdLines[2] = "";
  lcdLines[3] = "";


  cursorCol = 0;
  cursorRow = 0;
}


// ======================================================
// LOOP
// ======================================================

void loop() {

  handleButtons();


  if (trainingMode) {

    handleTrainingMode();
  }

  else {

    handleKeyer();

    handleDecoder();
  }
}

