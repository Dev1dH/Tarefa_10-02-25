/*
  ATIVIDADE EMBARCATECH - TAREFA AULA SÍNCRONA 10/02
  Aluno: Devid Henrique Pereira dos Santos
  Matrícula: TIC370100380
*/

// Bibliotecas
//#include <stdio.h>        // Biblioteca
//#include <stdlib.h>         // Biblioteca
#include "pico/stdlib.h"    // Biblioteca padrão pio SDK
#include "hardware/adc.h"   // Biblioteca para conversor analógico-digital
#include "hardware/i2c.h"   // Biblioteca para habilitar o I2C
#include "lib/ssd1306.h"    // BIblioteca para configuração do display OLED
//#include "pico/bootrom.h" // Biblioteca
#include "hardware/pwm.h"   // Biblioteca do PWM
#include "hardware/timer.h" // Biblioteca para funções de temporização e contagem de tempo

// Define os pinos display
#define I2C_PORT i2c1      //
#define I2C_SDA 14         //
#define I2C_SCL 15         // 
#define endereco 0x3C      //

// Define os pinos do Joystick
#define JOYSTICK_X_PIN 26  // GPIO para eixo X
#define JOYSTICK_Y_PIN 27  // GPIO para eixo Y
#define JOYSTICK_PB 22     // GPIO para botão do Joystick

// Define os pinos dos LEDs
#define LED_R 13    // Pino do LED vermelho  
#define LED_B 12    // Pino do LED azul
#define LED_G 11    // Pino do LED verde

// Define o pino do botão A
#define BOT_A 5 

#define tempo 2500

// Variáveis globais
static volatile uint a = 1;
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)

bool cor = true;

ssd1306_t ssd; // Inicializa a estrutura do display

// Função para configurar o display
void setup_display(){

  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA); // Pull up the data line
  gpio_pull_up(I2C_SCL); // Pull up the clock line
  
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd); // Configura o display
  ssd1306_send_data(&ssd); // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);
}

// Função de interrupção com debouncing
void gpio_irq_handler(uint gpio, uint32_t events)
{
    // Obtém o tempo atual em microssegundos
    uint32_t current_time = to_us_since_boot(get_absolute_time());
  
    if (current_time - last_time > 200000) // 200 ms de debouncing
    {
        last_time = current_time; // Atualiza o tempo do último evento
    }

    if(gpio_get(BOT_A)==0){
      gpio_put(LED_G, !gpio_get(LED_G));
    }
    
    else if(gpio_get(JOYSTICK_PB)==0){
      gpio_put(LED_B, !gpio_get(LED_B));
      gpio_put(LED_B, !gpio_get(LED_B));  
    }
}

// Função para configurar as GPIOS
void setup_gpios(){

  stdio_init_all();

  // Inicializa os pinos dos LEDs
  gpio_init(LED_B);
  gpio_init(LED_R);
  gpio_init(LED_G);

  // Define os LEDs como saída
  gpio_set_dir(LED_B, GPIO_OUT);
  gpio_set_dir(LED_R, GPIO_OUT);
  gpio_set_dir(LED_G, GPIO_OUT);

  // Inicializa e configura o botão A
  gpio_init(BOT_A);
  gpio_set_dir(BOT_A, GPIO_IN);
  gpio_pull_up(BOT_A);

  // Inicializa e configura o botão do joystick
  gpio_init(JOYSTICK_PB);
  gpio_set_dir(JOYSTICK_PB, GPIO_IN);
  gpio_pull_up(JOYSTICK_PB);

}

// Função para configurar o PWM
void setup_pwm(){

// Configura o PWM para o LED azul
  gpio_set_function(LED_B, GPIO_FUNC_PWM);      // Configura o pino do LED azul como saída PWM
  uint slice1 = pwm_gpio_to_slice_num(LED_B);   // Obtém o slice do PWM associado ao pino do LED azul
  pwm_set_clkdiv(slice1, 25);                   // Define o divisor de clock do PWM
  pwm_set_wrap(slice1, 4096);                   // Configura o valor máximo do contador (período do PWM)
  pwm_set_gpio_level(LED_B, 0);                 // Define o nível inicial do PWM para o LED azul
  pwm_set_enabled(slice1, true);                // Habilita o PWM no slice correspondente ao LED azul

  // Configura o PWM para o LED vermelho
  gpio_set_function(LED_R, GPIO_FUNC_PWM);      // Configura o pino do LED vermelho como saída PWM
  uint slice2 = pwm_gpio_to_slice_num(LED_R);   // Obtém o slice do PWM associado ao pino do LED vermelho
  pwm_set_clkdiv(slice2, 25);                   // Define o divisor de clock do PWM
  pwm_set_wrap(slice2, 4096);                   // Configura o valor máximo do contador (período do PWM)
  pwm_set_gpio_level(LED_R, 0);                 // Define o nível inicial do PWM para o LED vermelho
  pwm_set_enabled(slice2, true);                // Habilita o PWM no slice correspondente ao LED vermelho

}

int main()
{
  setup_display();
  setup_gpios();
  setup_pwm();
  
  // Configura o adc do joystick
  adc_init();                       // Inicializa o adc
  adc_gpio_init(JOYSTICK_X_PIN);    // adc do joystick eixo x
  adc_gpio_init(JOYSTICK_Y_PIN);    // adc o joystick eixo y 
  
  // Configura a função de interrupção
  gpio_set_irq_enabled_with_callback(BOT_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  gpio_set_irq_enabled_with_callback(JOYSTICK_PB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

  uint16_t centro = 2048;
  uint16_t margem = 50; // Margem de ±50 para considerar a região central

  while (true)
  {
     
    adc_select_input(0);        // Seleciona o ADC para eixo X. O pino 26 como entrada analógica
    uint16_t adc_value_x = adc_read();   // Variável para guardar o valor do adc do eixo X
    adc_select_input(1);        // Seleciona o ADC para eixo Y. O pino 27 como entrada analógica
    uint16_t adc_value_y = adc_read();   // Variável para guardar o valor do adc do eixo y   
    
    uint8_t valor_x = adc_value_x/22; // Converte o valor do adc em um inteiro de 8 bits na resolução do display
    uint8_t valor_y = adc_value_y/36; // Converte o valor do adc em um inteiro de 8 bits na resolução do display

    // Verifica se os valores do ADC estão na região central
    if (abs(adc_value_x - centro) <= margem && abs(adc_value_y - centro) <= margem)
    {
        // Apaga os LEDs
        pwm_set_gpio_level(LED_R, 0); // Desliga o LED vermelho apagado
        pwm_set_gpio_level(LED_B, 0); // Desliga o LED azul apagado
    }
    else
    {
        // Ajusta o brilho dos LEDs com base nos valores do ADC
        pwm_set_gpio_level(LED_R, adc_value_x); // Varia o brilho do LED vermelho proporcional ao adc do eixo x
        pwm_set_gpio_level(LED_B, adc_value_y); // Varia o brilho do LED azul proporcional ao adc do eixo Y
    }

    ssd1306_fill(&ssd, !cor); // Limpa o display  
    ssd1306_rect(&ssd, valor_x, valor_y, 8, 8, cor, !cor); // Desenha o quadrado de 8x8       
    ssd1306_send_data(&ssd); // Atualiza o display
    
  }

  return 0;
}