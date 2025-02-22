#include <stdio.h>     
#include <stdlib.h>           
#include "pico/stdlib.h"    // Biblioteca padrão pio SDK
#include "hardware/adc.h"   // Biblioteca para conversor analógico-digital
#include "hardware/i2c.h"   // Biblioteca para habilitar o I2C
#include "bibli/ssd1306.h"  // BIblioteca para configuração do display OLED
#include "hardware/pwm.h"   // Biblioteca do PWM
#include "hardware/timer.h" // Biblioteca para funções de temporização e contagem de tempo

// Define os pinos do display
#define I2C_PORT i2c1      
#define I2C_SDA 14         
#define I2C_SCL 15         
#define endereco 0x3C      

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
static volatile bool led_verde_estado = false; // Estado do LED Verde
static volatile uint8_t estilo_borda = 0; // Alternância de borda

bool cor = true;

ssd1306_t ssd; // Inicializa a estrutura do display

// Função para configurar o display
void setup_display(){

  // I2C Inicialização usando 400Khz.
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

        if (gpio == BOT_A) {
            gpio_put(LED_G, !gpio_get(LED_G));
        }
        else if (gpio == JOYSTICK_PB) {
            // Alterna o estado do LED Verde
            led_verde_estado = !led_verde_estado;
            gpio_put(LED_G, led_verde_estado);

            // Alterna o estilo da borda do display
            estilo_borda = (estilo_borda + 1) % 3;
        }
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
  adc_init();                       
  adc_gpio_init(JOYSTICK_X_PIN);    
  adc_gpio_init(JOYSTICK_Y_PIN);    
  
  // Configura a função de interrupção
  gpio_set_irq_enabled_with_callback(BOT_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  gpio_set_irq_enabled_with_callback(JOYSTICK_PB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

  uint16_t centro = 2048;
  uint16_t margem = 50; 

  while (true)
  {
    adc_select_input(0);        
    uint16_t adc_value_x = adc_read();   
    adc_select_input(1);        
    uint16_t adc_value_y = adc_read();   
    
    uint8_t valor_x = adc_value_x / 22;
    uint8_t valor_y = adc_value_y / 36;

    if (abs(adc_value_x - centro) <= margem && abs(adc_value_y - centro) <= margem)
    {
        pwm_set_gpio_level(LED_R, 0);
        pwm_set_gpio_level(LED_B, 0);
    }
    else
    {
        pwm_set_gpio_level(LED_R, adc_value_x);
        pwm_set_gpio_level(LED_B, adc_value_y);
    }

    ssd1306_fill(&ssd, !cor);

    switch (estilo_borda) {
        case 0: ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, true, false); break;
        case 1: ssd1306_rect(&ssd, 2, 2, WIDTH - 4, HEIGHT - 4, true, false); break;
        case 2: ssd1306_rect(&ssd, 4, 4, WIDTH - 8, HEIGHT - 8, true, false); break;
    }

    ssd1306_rect(&ssd, valor_x, valor_y, 8, 8, cor, !cor);
    ssd1306_send_data(&ssd);
  }

  return 0;
}
