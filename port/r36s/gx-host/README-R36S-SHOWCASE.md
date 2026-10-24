# CubeEvil4 R36S | Demonstração de jogabilidade 4:3 e áudio (experimental)

## O que esta versão faz

Esta **versão de apresentação experimental** é destinada ao R36S com
DarkOSRE Linux aarch64 (RK3326 4x Cortex-A35, GPU Mali-G31 GLES3.2).
Ela utiliza o jogo GameCube **G4BE08** já instalado legitimamente pelo
testador; **não inclui jogos, imagens de discos, módulos do Resident Evil
4, BIOS, memcards nem coreproc recompilado**.

É um frontend GX **separado**: não substitui os executáveis de diagnóstico
e não modifica a instalação estável, as bibliotecas de jogo, os módulos,
a pasta `data/` ou os saves.

- **Definição interna 480×396**, a preferida do testador;
- **Zoom para tela 4:3** por padrão: remove o letterbox ao expandir o
  centro da imagem. Mantém proporções geométricas, mas **corta cerca
  de 12,5% de cada lateral**, o que pode ocultar parcialmente HUD ou
  objetos nas bordas;
- **Esticado 4:3** opcional: remove só as faixas de cima/baixo e preserva
  a largura da imagem, mas alarga/estica verticalmente o enquadramento;
- **Original** opcional: mostra o frame GameCube íntegro, barras inclusas;
- **Áudio estéreo real**: toca pelo SDL2 (amostras PCM originais de
  32.000 Hz S16, dois canais) com um feeder thread independente da GPU
  e fila limitada para evitar atraso de vários segundos;
- **Menu próprio para sair do jogo** pelo SELECT/BACK ou GUIDE/Home
  se mapeado, ou teclas ESC/F1. Se o firmware não mapear SELECT como
  SDL_BACK, segure **START+L1+R1 por 1 segundo**;
- **No menu**, A confirma saída, B volta e Y percorre
  zoom -> esticado -> original -> zoom. Em teclado:
  ENTER/Z confirma, BACKSPACE/X cancela e Y troca o modo;
- **Sem limitação de 180 segundos:** sessão até 2h, ou saída antes pelo
  controle;
- Compilação `-O3 -mcpu=cortex-a35 -mtune=cortex-a35 -flto=auto`;
- **Sem profiler de GPU**, fences de diagnóstico, logs de FPS a
  cada 5 segundos nem rastreamento de sinais do frontend. O script
  silencia stdout/stderr por padrão. Isso não elimina o trabalho
  interno de formatação/logging do núcleo proprietário; essa parte
  não foi recompilada;
- **Preserva o `glFinish` antes do `SDL_GL_SwapWindow`** com VSync1,
  comportamento imprescindível para a estabilidade na Mali-G31.

Importante: **zoom 4:3 NÃO é uma câmera widescreen convertida
nativamente para 4:3**. É transformação da imagem final XFB.
Um hack real de FOV/HUD exigiria modificações adicionais no código
do jogo que ainda não foram desenvolvidas.

## Instalação

GitHub Actions: workflow `R36S - 4by3 SDL audio showcase`.

Baixe o artifact `CubeEvil4-R36S-Demo-43-Audio-ARM64`.
Extraia em `/roms/ports`. O ZIP acrescenta somente:

- `cubeevil4_live_gx/r36s_game_showcase`
- `RE4-R36S-Demonstracao-43.sh`
- `LEIA-ME-R36S-DEMO.md`

**Preserve absolutamente tudo** da instalação preexistente
(`r36s_game_boot`, `r36s_game_a35_scalable`, `re4_coreproc`,
`libre4_main.so`, `libre4_platform.so`, `modules/`,
`data/disc0/`, `data/disc1/`, `data/memcard/`).

Apenas para máquinas com os arquivos de uma instalação original
compatível do port. O launcher verifica as pastas antes de iniciar.

## Usando os comandos do R36S

| Ação | Comando |
|---|---|
| Abrir menu de saída | SELECT/BACK (ou GUIDE/HOME quando mapeado) |
| Atalho alternativo | START + L1 + R1 por 1 segundo |
| Voltar ao jogo | B no menu |
| Sair e voltar ao DarkOSRE | A no menu |
| Alternar modo da imagem | Y dentro do menu |
| Abrir no teclado | ESC ou F1 |

A abertura do menu solicita `re4ipc_pause(1)` ao núcleo e envia
controles neutros ao jogo. Ao voltar, solicita `re4ipc_pause(0)`.
Dê preferência a abrir o menu antes de uma transição de sala/cutscene.
Um pause pode falhar ou atrasar caso a thread de jogo esteja bloqueada
em outro trabalho; teste no hardware físico.

## Configuração opcional

No shell do R36S, antes de abrir o launcher:

```sh
RE4_SHOWCASE_ASPECT=zoom sh /roms/ports/RE4-R36S-Demonstracao-43.sh
RE4_SHOWCASE_ASPECT=stretch sh /roms/ports/RE4-R36S-Demonstracao-43.sh
RE4_SHOWCASE_ASPECT=original sh /roms/ports/RE4-R36S-Demonstracao-43.sh
```

O padrão é zoom. O botão **Y no menu** também troca os modos durante
a sessão.

Para investigar algum erro de início, execute como desenvolvedor:

```sh
RE4_SHOWCASE_VERBOSE=1 sh /roms/ports/RE4-R36S-Demonstracao-43.sh
```

O pacote **não produz log de diagnóstico no uso normal**. Mesmo em modo
verbose, o binário foi compilado sem instrumentação de GPU/quadros;
algumas mensagens de erro da biblioteca e do núcleo ainda podem surgir.

O SDL/ALSA do DarkOSRE deve reconhecer um dispositivo de saída PCM
estéreo de 32000 Hz. Se não reconhecer, o jogo prossegue em silêncio
para não bloquear o núcleo. **Áudio audível e sincronismo ainda não
foram comprovados no R36S real** e podem exigir configurar o mixer
do próprio firmware ou uma amostragem adaptada.

## Resultado dos testes prévios de buffers (não do novo áudio!)

Os relatórios anexados de 180 segundos:

| Estatística | Base glBufferData | Reuso glBufferSubData |
|---|---:|---:|
| Quadros apresentados | 1701 | 1915 |
| FPS ponderado em janelas com 400+ draws/quadro | **6,28** | **6,23** |
| Alocações de buffers GL | 3402 | 2 |
| Tempo de parede dos uploads GL | 3,91s | 3,63s |
| GL/shader/IPC errors | 0 | 0 |
| Finalização | 0 | 0 |

Ambos percorreram cargas diferentes. A diferença de FPS nas
janelas pesadas **não demonstrou ganho de jogabilidade**.
A versão de demonstração adota o envio original, mais simples e
confiável, e mantém o shader cache em disco. Não use a quantidade
de quadros de sessões diferentes como prova de aceleração.

## Distribuição e direitos autorais

Este projeto é fan-made, sem vínculo com Capcom, Nintendo ou CRI.
O `LICENSE` do projeto é explícito: CC0 cobre scripts, ferramentas
e documentação do projeto, **não licencia o código reconstruído do
jogo, SDK ou middleware**. Não há autorização automática para
redistribuir uma demo jogável que venha acompanhada de código
reconstruído, executáveis originados desse código, arquivos de disco,
texturas, música, vídeos ou assets comerciais.

O artifact contém apenas o frontend e documentação, **sem**
`re4_coreproc`, `libre4_*.so`, RELs nem arquivos proprietários.
Ele não executa o jogo por si só; o testador precisa fornecer sua
própria instalação compatível legalmente obtida. Mesmo ao distribuir
o frontend, confira os direitos/licenças de todos os componentes
que serão repassados. Não compartilhe seu diretório completo
`cubeevil4_live_gx` nem imagens de discos com grupos públicos.

Para buscar apoio e mostrar o avanço, uma opção simples é
gravar um vídeo da sua execução no R36S e disponibilizar a
documentação, o código das modificações e os testes com números
honestos, sem repassar material protegido.

## Validação física pendente

O CI compila ARM64 e checa a arquitetura. Isso **não testa**
som real, mapeamento SELECT no DarkOSRE, pause, HUD no zoom 4:3
nem o framebuffer no Mali. Antes de recomendar o artifact aos
grupos, rode-o no próprio R36S, teste menu e áudio com título,
efeitos sonoros, tiro, caminho na primeira sala e saída limpa.

Se falhar, use imediatamente `RE4-R36S-Qualidade-75.sh` ou
`RE4-R36S-Estavel.sh` e preserve os saves.
