# Resident Evil 4 GameCube no R36S — menu de saída sem acionamentos involuntários

## Qual era o problema

A demonstração anterior considerava o botão físico RAW `0` como
`SDL_CONTROLLER_BUTTON_GUIDE/HOME` quando o firmware não fornecia
um mapeamento completo. Mas o mesmo botão RAW `0` corresponde ao
**A** em diversos mapeamentos. Assim, ao apertar A durante o jogo, o
programa recebia "HOME" e **abria o menu de saída sem querer**.
Os eventos instantâneos `SDL_JOYBUTTONDOWN` e
`SDL_CONTROLLERBUTTONDOWN` do mesmo botão também duplicavam
o acionamento.

A correção **não utiliza mais HOME nem aliases RAW 0,6,8 para
SELECT por padrão**, e **nenhum toque curto em botão físico**
pode abrir o menu. A entrada é lida pelo SDL GameController com
fallback de joystick apenas para índices comprovados pelo binding
do SDL ou configurados manualmente, exceto START RAW 9 de
emergência se SDL não oferecer mapeamento.

## Controles agora (sem menu aparecendo sozinho)

| Controle | Ação |
|---|---|
| **START pressionado continuamente por 4 segundos** | Abre o menu uma vez |
| **START pressionado continuamente por 8 segundos** | Sai diretamente para o DarkOSRE |
| **SELECT pressionado por 2 segundos** | Abre/fecha o menu (se SELECT estiver mapeado) |
| **START + SELECT por 2 segundos** | Sai diretamente (se ambos estiverem mapeados) |
| A no menu | Confirma saída |
| B no menu | Volta a jogar |
| ESC/F1 no teclado | Abre/fecha o menu |

**A, B, START ou SELECT usados normalmente no jogo NÃO abrem o menu
com um toque curto.** Segurar START por 4s abre o menu apenas uma vez;
se continuar segurando até 8s, sai diretamente.

Ao abrir a aplicação, os atalhos ficam desarmados até detectar
START e SELECT soltos por pelo menos 350 ms. Isso evita um comando
remanescente do menu Ports provocar um falso acionamento.

## Instalação

Dois artifacts são publicados pelo workflow:
- `CubeEvil4-R36S-Demo-Sem-Menu-Fantasma-ARM64`: **atualização
  mínima**, se sua instalação já tem núcleo e arquivos completos.
- `CubeEvil4-R36S-RESTAURO-COMPLETO-SEM-MENU-FANTASMA`: núcleo e
  módulos ARM64 compilados do port + renderizador estável +
  demonstração corrigida + verificador de arquivos.

Extraia UM dos ZIPs diretamente em `/roms/ports`, **mesclando
pastas**, sem apagar a pasta original `data/`, o núcleo nem saves.
O script que aparece no menu Ports continua chamando
`RE4-R36S-Demonstracao-Tela-Cheia.sh`.

O novo binário se chama `r36s_game_showcase_menuquiet`.
O script antigo passa a usar esse executável, preservando
`r36s_game_showcase_exitfix`, `r36s_game_boot` e os outros
testes existentes para permitir retorno à versão anterior.

Características do modo demo mantidas:
- 640x480 **tela cheia esticada**, sem cortar as laterais;
- render interno 480x396 (75%);
- áudio SDL2 estéreo PCM 32 kHz;
- não desenha debug do GameCube (`[PRINT_PAGE]=0` via overlay
  temporário sem editar os dados do usuário);
- sem relatórios ou FPS por cima da imagem;
- modo GPU Mali 5 = VSync 1 + `glFinish` antes do swap;
- compilador `-O3 -mcpu=cortex-a35 -mtune=cortex-a35 -flto=auto`.

## Se os botões ainda não forem reconhecidos

O firmware DarkOSRE pode mapear os botões físicos em índices RAW
diferentes. O launcher aceita variáveis explícitas, por exemplo:

```sh
RE4_SHOWCASE_RAW_SELECT=8 RE4_SHOWCASE_RAW_START=9 \
  sh /roms/ports/RE4-R36S-Demonstracao-Tela-Cheia.sh
```

Ou registre os eventos reais sem mostrar debug na tela:

```sh
RE4_SHOWCASE_INPUT_DIAG=1 sh /roms/ports/RE4-R36S-Demonstracao-Tela-Cheia.sh
```

Depois de apertar A, B, START e SELECT uma vez, o arquivo
`/roms/ports/cubeevil4_live_gx/logs/diagnostico_controles_demo_darkos.txt`
mostra `R36S_RAW_JOYBUTTON_DOWN DEVICE=... BUTTON=...`.
Assim conseguimos definir o mapeamento exato. O arquivo não é
produzido na execução normal.

## Instalação sem jogos comerciais

O pacote completo contém apenas o código reconstruído e os módulos
compilados que já estavam nos artifacts do projeto. NÃO inclui
`data/disc0`, ISO, áudio/texturas originais ou saves. Se você
apagou arquivos comerciais do Resident Evil 4 G4BE08, restaure-os
de sua própria extração/backup, preservando a pasta `data/memcard`.

Teste automatizado cobre 70 pressionamentos alternados de A/B no jogo
sem abrir o menu, comandos longos, saída de emergência,
ausência de duplo acionamento, estado inicial e mapeamento explícito.
**A validação no joystick físico do R36S ainda é necessária.**
