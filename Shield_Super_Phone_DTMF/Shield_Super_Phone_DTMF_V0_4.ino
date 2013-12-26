/**
 * Para monitorar o sinais recebidos na linha.
 *
 */

#include "limits.h"
#include "stdio.h"


#define PIN_LINHA_TOCANDO     3
#define SIGNAL_LINHA_TOCANDO  1
#define SIGNAL_TYPE_LINHA_TOCANDO CHANGE

#define PIN_LINHA_EM_USO     2
#define SIGNAL_LINHA_EM_USO  0
#define SIGNAL_TYPE_LINHA_EM_USO CHANGE 

#define PIN_LINHA_LIVRE       4

#define PIN_HOOK_CONTROL      8

#define PIN_DTMF_DV           6
#define PIN_DTMF_D0           7
#define PIN_DTMF_D1           8
#define PIN_DTMF_D2           9
#define PIN_DTMF_D3           10

#define PIN_LED_TOCANDO      13
#define PIN_LED_LIVRE        11
#define PIN_LED_OCUPADO      12

#define PIN_INPUT_AUDIO_1    A0


volatile unsigned long disparoLinhaEmUso = 0; // ULONG_MIN;
volatile unsigned long segundoDisparoLinhaEmUso = 0; //ULONG_MIN;
volatile unsigned long disparoLinhaLivre = 0;
volatile unsigned long tempoDisparoLinhaTocando = 0;

volatile boolean linhaEmUso = false ;
volatile boolean linhaTocando = false ;
volatile boolean linhaTocandoRing = false ;
volatile boolean linhaComPico = false;

const char dtmfTxt[] = {
  'D','1','2','3','4','5','6','7','8','9','0','*','#','A','B','C'};


void setup() {
  Serial.begin(115200);

  pinMode(PIN_LINHA_TOCANDO,INPUT_PULLUP);
  pinMode(PIN_LINHA_LIVRE,INPUT_PULLUP);
  pinMode(PIN_LINHA_EM_USO,INPUT_PULLUP);

  pinMode(PIN_HOOK_CONTROL,OUTPUT);

  pinMode(PIN_LED_TOCANDO,OUTPUT);
  pinMode(PIN_LED_LIVRE,OUTPUT);
  pinMode(PIN_LED_OCUPADO,OUTPUT);

  //                          1         funcao           CHANGE
  attachInterrupt(SIGNAL_LINHA_TOCANDO, telefoneTocando, SIGNAL_TYPE_LINHA_TOCANDO);
  //                          0         funcao              CHANGE
  attachInterrupt(SIGNAL_LINHA_EM_USO, telefoneLinhaEmUso, SIGNAL_TYPE_LINHA_EM_USO);

  pinMode(PIN_DTMF_DV,INPUT);
  pinMode(PIN_DTMF_D3,INPUT);
  pinMode(PIN_DTMF_D2,INPUT);
  pinMode(PIN_DTMF_D1,INPUT);
  pinMode(PIN_DTMF_D0,INPUT);
  
  resetaSinais();

}


char *bfrtime = (char*)malloc(sizeof(char)*150);

void loop(){
  unsigned long time = millis();

  boolean estadoPinLinhaLivre = digitalRead(PIN_LINHA_LIVRE);
  boolean estadoPinLinhaEmUso = digitalRead(PIN_LINHA_EM_USO);
  boolean estadoPinLinhaTocando = digitalRead(PIN_LINHA_TOCANDO);

  char *msg;
  if(linhaComPico)
    msg = "** Pico**";
  else if(estadoPinLinhaLivre && estadoPinLinhaEmUso && estadoPinLinhaTocando)
    msg = "*PROBLEMA*";
  else if(!linhaEmUso) 
    msg = " Livre!!!";
  else 
    msg = "EmUso!!!";

  byte dtmf;
  boolean dtmfDV = digitalRead(PIN_DTMF_DV);
  if(dtmfDV){
    dtmfDV = 
      (digitalRead(PIN_DTMF_D3) << 3)||
      (digitalRead(PIN_DTMF_D2) << 2)||
      (digitalRead(PIN_DTMF_D1) << 1)||
      (digitalRead(PIN_DTMF_D0) << 0);
  }

  sprintf(bfrtime,  
  "!!%9lu!!Toque: %9lu!%10s!!Livre:%9lu!!Ocupado:%9lu!! %10s !! %i ! %i ! %i ! %4i!DTMF: %1u!%c !!%9lu!!",
  time,
  tempoDisparoLinhaTocando,
  (linhaTocando?"Tocando!!":" "),
  disparoLinhaLivre,
  disparoLinhaEmUso,
  msg,
  estadoPinLinhaLivre,
  estadoPinLinhaEmUso,
  estadoPinLinhaTocando,
  analogRead(PIN_INPUT_AUDIO_1),
  dtmfDV,
  dtmfDV?dtmfTxt[dtmf]:' ',
  time
    );


  Serial.println(bfrtime);

  digitalWrite(PIN_LED_TOCANDO, linhaTocando);
  //  digitalWrite(PIN_LED_LIVRE,!digitalRead(PIN_LINHA_LIVRE)); // usar este se quiser que o LED linha Livre conecte direto com sinal Linha Livre!!
  digitalWrite(PIN_LED_LIVRE,  !linhaEmUso);
  digitalWrite(PIN_LED_OCUPADO, linhaEmUso);

  if((millis() - tempoDisparoLinhaTocando) > 60 && linhaTocando){
    resetaSinais();
  } 
}

inline void resetaSinais(){
  linhaTocando = false;
  linhaTocandoRing = false;

  linhaEmUso = false;
  
  linhaComPico = false;
}

/* Variavel que armazena quando ocorreu o ultimo disparo 
 * da funcao telefoneTocando().
 */
volatile unsigned long ultimoTempoDisparoTelefoneTocando, picoTempoDisparoLinhaTocando, picoTempoDisparoLinhaEmUso;

/**
 * Funcao chamada quando o acoplamento sinaliza que o telefone esta tocando.
 *
 */
void telefoneTocando(){
  unsigned long tempoDisparo = millis();

  linhaComPico = ((tempoDisparo - picoTempoDisparoLinhaTocando)<15);

  if(linhaComPico){
    picoTempoDisparoLinhaTocando = tempoDisparo;

  }
  else if(digitalRead(!PIN_LINHA_EM_USO) && digitalRead(!PIN_LINHA_LIVRE) && (tempoDisparo - ultimoTempoDisparoTelefoneTocando) > 60){
    linhaTocando = true;
    linhaTocandoRing = !linhaTocandoRing;

    linhaEmUso = true;

    tempoDisparoLinhaTocando = tempoDisparo;
    ultimoTempoDisparoTelefoneTocando = tempoDisparo;
  }
}

/**
 * Esta funcao e chamada quando o acoplamento sinaliza 
 * que a linha ficou em uso.
 * 
 */
void telefoneLinhaEmUso(){
  unsigned long tempoDisparo = millis(); 

  linhaComPico = ((tempoDisparo - picoTempoDisparoLinhaEmUso)<15);

  if(linhaComPico){
    picoTempoDisparoLinhaEmUso = tempoDisparo; 
  }
  else if(digitalRead(PIN_LINHA_LIVRE) && !digitalRead(PIN_LINHA_EM_USO) && !linhaTocando){
    linhaEmUso =  true; 
    disparoLinhaEmUso = tempoDisparo;
  } 
  else if(!digitalRead(PIN_LINHA_LIVRE) &&  digitalRead(PIN_LINHA_EM_USO) && !linhaTocando){
    linhaEmUso =  false; 
    disparoLinhaLivre = tempoDisparo;
  }
  else if(!linhaTocando){
    segundoDisparoLinhaEmUso = tempoDisparo;
  }
}











