# CubeEvil4 R36S — Correção da saída por botões físicos Linux (DarkOSRE)

## Problema real

O usuário confirmou em aparelho físico que **nenhuma** das combinações
START/SELECT das builds anteriores funcionou no jogo otimizado A35 O3.
A versão antiga só consultava `SDL_GameControllerGetButton(START/BACK)`
e o índice RAW 9 do joystick. Em diferentes firmwares e clones
do RK3326, o pad pode ser um joystick genérico, reportar índices
físicos diferentes, ou ter um daemon de hotkeys que intercepta eventos.

Tentativas anteriores que presumiam o botão RAW 0 como HOME causaram
um menu de saída fantasma ao pressionar A. **Não voltamos a adivinhar
índices de A/HOME.**

## Correção implementada

- **Leitura independente de SDL:** `r36s_showcase_evdev_exit.h`
  abre cada `/dev/input/event[0-9]*` SOMENTE para leitura e identifica
  capacidades de entrada Linux `BTN_START=315` e
  `BTN_SELECT=314` usando `EVIOCGBIT`. Também aceita os códigos
  pareados mais antigos `BTN_TRIGGER_HAPPY1=704` /
  `BTN_TRIGGER_HAPPY6=709` ou `BTN_TRIGGER_HAPPY2=705`.
- A cada iteração da partida, `EVIOCGKEY` consulta o **estado atual**
  dos botões físicos (não depende de receber o evento
  `SDL_CONTROLLERBUTTONDOWN`). Isso pode funcionar mesmo quando
  um serviço do firmware retém os eventos exclusivos. **Se o nó de
  entrada não for legível ou o firmware não expuser o código de botão,
  o método também não funcionará.**
- Mantém leitura SDL GameController e os números físicos fornecidos
  explicitamente pelo usuário (env `RE4_SHOWCASE_RAW_START` e
  `RE4_SHOWCASE_RAW_SELECT`). O fallback RAW 9 presumido foi
  **removido** para não acionar o menu em botões errados.
- **SIGUSR1 / SIGTERM / SIGINT:** o executável registra um flag
  assíncrono seguro para solicitar saída normal. O loop encerra
  a reprodução de áudio, desliga o núcleo IPC e libera a janela SDL
  e contexto EGL, em vez de matar os processos abruptamente.
  É o caminho remoto alternativo independente dos botões.
- Nenhum overlay ou contador de depuração na tela, e o logger
  de controles só é ativado por solicitação.

## Comandos no controle

São mantidos os gestos deliberados da versão sem menu fantasma:

| Controle reconhecido | Ação |
|---|---|
| Segurar **START 4 segundos** | Abrir o menu de saída |
| Segurar **START 8 segundos** | Sair diretamente para DarkOSRE |
| Segurar **SELECT 2 segundos** | Abrir ou fechar menu |
| Segurar **START + SELECT 2 segundos** | Sair diretamente |
| **A ou B** no menu | Sair ou voltar |
| **ESC ou F1** no teclado | Abrir ou fechar o menu |

O gesto por tempo evita menu involuntário ao apertar A,
B ou START brevemente na partida. Os gestos só são armados após
START e SELECT serem detectados soltos por 350 ms na inicialização.

## Instalação (atualização pequena)

Baixe o artifact GitHub Actions:
**`CubeEvil4-R36S-CORRECAO-SAIDA-EVDEV-ARM64`**.

Extraia todo o conteúdo em `/roms/ports` com mesclagem de pastas.
O pacote acrescenta SOMENTE:

```text
/roms/ports/
  RE4-R36S-Nucleo-A35-O3.sh                # script da build O3, atualizado
  RE4-R36S-Demonstracao-Tela-Cheia.sh      # script da demo antiga, atualizado
  RE4-R36S-Mapear-Botoes.sh                # diagnóstico independente do jogo
  LEIA-ME-CORRECAO-SAIDA.md
  cubeevil4_live_gx/
    r36s_game_showcase_evdev_exit          # novo frontend
    r36s_showcase_input_probe              # leitura de índices físicos
```

**NÃO inclui nem altera:** `core_a35_o3/re4_coreproc`,
`core_a35_o3/modules/`, `re4_coreproc`, `libre4_*.so`,
`data/disc0`, `data/memcard/card-a.bin`, nem shader cache.
A implementação mantém a resolução 480x396, fullscreen 640x480
esticado, SDL áudio 32kHz, sem debug, Cortex-A35 e
**swap mode 5 com glFinish antes do swap**. A nova build compartilha
o núcleo O3 validado por testes, mas não substitui a build anterior.

### Se START ainda não funcionar

Abra `RE4-R36S-Mapear-Botoes.sh` pelo menu Ports.
Durante a leitura de 12 segundos, pressione uma vez SELECT, START,
A e B. A ferramenta cria:

`/roms/ports/cubeevil4_live_gx/logs/diagnostico_botoes_fisicos_r36s.txt`

Os dados registram `SDL_RAW_BUTTON ... BUTTON=...` e
`LINUX_KEY ... CODE=... STATE=...`, além dos dispositivos
abertos ou sem permissão. **Envie o TXT para que possamos mapear
exatamente seu clone e firmware**, sem adivinhações.
Ele roda fora do jogo e nunca mexe nos saves.

Por SSH, com a versão atualizada em execução, há também uma
saída independente do controle físico:

```sh
killall -USR1 r36s_game_showcase_evdev_exit
```

Isso **solicita** a finalização normal do frontend. Requer
que o loop do jogo ainda esteja respondendo; um travamento no
driver gráfico pode impedir a saída até o sistema recuperar.
A mesma versão reage a SIGTERM de maneira controlada.

## Limitações e validação

O CI executa testes para distinguir BTN_START/BTN_SELECT de A/B/
direcionais e verifica as combinações de tempo de saída, teste do
config original, compilação ELF AArch64 de frontend + ferramenta.
**Não valida os botões reais do DarkOSRE, a presença das permissões
de /dev/input ou a estabilidade GPU no aparelho do usuário**.
Este ajuste ainda precisa de teste no R36S real.

O pacote não distribui dados ou executáveis do Resident Evil 4:
somente novo frontend e ferramentas do port. A instalação original
compatível e legalmente obtida continua necessária.
