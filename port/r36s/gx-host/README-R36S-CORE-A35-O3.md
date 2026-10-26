# Resident Evil 4 GameCube no R36S — otimização real do núcleo do jogo (A35 + O3 + GX FIFO)

## O que foi descoberto no estudo do repositório original

O repositório `adonis-singh/re4` **já é a fonte principal** da nossa
cópia `dehalbuq/CubeEvil4-Android`: os 1.051 arquivos de `src/`,
492 de `include/` e 465 de `config/` tinham **SHA Git idêntico** na
comparação. Não precisamos reescrever o motor, nem trocar por outro.
O port já compila esses arquivos originais para Linux ARM64 e recompõe
os 114 módulos originais do jogo.

Havia uma diferença importante de otimização:
- **Núcleo compilado anteriormente:** `RE4_OPT=2` (`-O2`) para
  `re4_coreproc`, `libre4_main.so` e os módulos reconstruídos.
- **Frontend GLES/SDL2:** `-O3 -mcpu=cortex-a35 -mtune=cortex-a35`,
  e LTO (apenas no frontend), em targets A35.

Esta experiência otimiza finalmente o **núcleo inteiro**, e não
somente o frontend GLS3.

## Otimizações implementadas

1. `RE4_R36S_A35_CORE=ON`: compilação Clang arm64 Linux com
   `-O3 -mcpu=cortex-a35 -mtune=cortex-a35` de **todos** os objetos do
   jogo, SDK portátil, adaptadores e módulos do RE4. Desabilitado por
   padrão; não modifica a build `-O2` antiga.
2. `RE4_R36S_FAST_FIFO=ON`: o código em
   `port/whole/runtime/gx_fifo.cpp` substitui cópias
   byte a byte em listas GX gravadas na memória do GameCube
   por `memcpy` contíguo, sem ultrapassar `PI top` nem limites
   de MEM1/aliases. O adaptador `gx_fifo_fast_copy.h`
   preserva a posição de escrita e o flag overflow byte a byte.
   Compila **somente** no núcleo experimental.
3. **Teste regressivo de 75.000 casos pseudoaleatórios:** compara
   a gravação original com a otimizada para diferentes tamanhos,
   cruzamentos de blocos e endereços inválidos; os bytes de memória,
   a posição de escrita, a quantização em 32 bytes e o resultado
   de overflow precisam ser iguais.
4. **Suíte original do núcleo reconstruído:** compila com as
   verificações do projeto, executa via QEMU AArch64 quando
   disponível: módulos e trampolins, save/memory card, testes
   de áudio/vídeo sintético, estruturas, loader e IPC.
   **Não significa que medimos FPS real no Mali.**
5. Preserva a precisão do original:
   `-ffp-contract=off`, sem `-ffast-math`, `-Ofast`,
   redução de polígonos, exclusão de vegetação, alteração
   de colisões ou mudanças no ciclo de gameplay.

As otimizações são plausíveis para reduzir trabalho na CPU
quad-core Cortex-A35, mas **um GPU bottleneck pode limitar
qualquer ganho de FPS**. Portanto, comparar no console físico
é indispensável.

## Instalação do artifact

Baixe `CubeEvil4-R36S-CORE-A35-O3-FASTFIFO-ARM64` da Action
`R36S - native original game engine A35 O3 GX fast FIFO`.
Extraia em `/roms/ports` mesclando as pastas existentes.

O ZIP instala **separadamente**, sem sobrescrever a base estável:

```text
/roms/ports/
  RE4-R36S-Nucleo-A35-O3.sh                 <-- DEMO com nucleo otimizado
  RE4-R36S-Teste-Nucleo-O2.sh               <-- benchmark referencia
  RE4-R36S-Teste-Nucleo-O3.sh               <-- benchmark A35/O3
  RE4-R36S-Comparar-Nucleo.sh
  LEIA-ME-A35-CORE-OPT.md
  cubeevil4_live_gx/
    r36s_game_showcase_a35_o3               <-- frontend de tela cheia sem menu fantasma
    r36s_game_a35_core_compare              <-- frontend de diagnostico igual nos 2 testes
    core_a35_o3/                            <-- diretório 100% SEPARADO
      re4_coreproc
      libre4_main.so
      libre4_host.so
      libre4_platform.so
      modules/
        libre4_*.so                         <-- 114 ou mais
    data/                                   <-- NÃO incluído; mantenha original
    cache/                                  <-- NÃO incluído; cache shader já existente
```

**Nada no artifact substitui** `cubeevil4_live_gx/re4_coreproc`,
`cubeevil4_live_gx/modules/`,
`cubeevil4_live_gx/libre4_main.so`,
`r36s_game_boot`,
`r36s_game_showcase_menuquiet`,
`data/disc0` ou `data/memcard/card-a.bin`.
Assim, se a nova compilação travar, basta usar a demo
`RE4-R36S-Demonstracao-Tela-Cheia.sh` antiga.

Requisitos: instalação base recuperada, dados G4BE08 do seu jogo
original em `cubeevil4_live_gx/data/disc0`, bibliotecas SDL2,
EGL/GLES3 da Mali-G31 e DarkOSRE aarch64 funcionando.

## Como testar no R36S

**Primeiro, comparativo de 180 segundos na mesma cena/rota**:

1. Abra `RE4-R36S-Teste-Nucleo-O2.sh`. Percorra o mesmo trecho,
   espere o término do teste e salve o relatório.
2. Abra `RE4-R36S-Teste-Nucleo-O3.sh`. Percorra **o mesmo trecho**
   e espere o término.
3. Envie os dois arquivos:

```text
/roms/ports/cubeevil4_live_gx/diagnostico_core_baseline_480x396_darkos.txt
/roms/ports/cubeevil4_live_gx/diagnostico_core_a35_480x396_darkos.txt
```

Ambos usam o **mesmo binário** `r36s_game_a35_core_compare`,
EFB 480×396, DOF/fog off, duas luzes, culling de distância
desabilitado, cache shader em disco, BaseVertex desativado
para manter o perfil anteriormente testado e modo de swap GPU5
(**glFinish antes do SDL_GL_SwapWindow**).

O primeiro teste usa o núcleo `-O2` já instalado na pasta raiz.
O segundo usa o núcleo `-O3 -mcpu=cortex-a35` e GX fastFIFO.
As estatísticas incluem `PERFORMANCE_WINDOW_SECONDS`,
`FRAMES`, `DRAWS`, `TRIANGLES`, `MemAvailable` e erros.
Compare **intervalos com carga semelhante**, não apenas frames
totais de sessões percorridas em tempos diferentes.

**Para testar o gameplay normal** após o comparativo,
abra `RE4-R36S-Nucleo-A35-O3.sh`: tela cheia esticada 4:3,
áudio SDL2 estéreo, debug do GameCube oculto e mesmas
regras de saída sem menu fantasma (START segurado 4 segundos
abre menu; 8 segundos sai diretamente). Teste mudanças de
sala, inimigos, objetos, inventário, tiros, sons e saves.

Se ocorrer travamento, desligue apenas o experimento:
volte ao script `RE4-R36S-Demonstracao-Tela-Cheia.sh`
que usa o antigo núcleo `-O2`.

## Limitações de desempenho

Nossos últimos A/B no R36S mostraram que economizar
4,24 milhões de atualizações XF ou milhares de alocações
OpenGL, isoladamente, **não melhorou claramente os FPS**
nos trechos de 400+ draws por quadro. O novo experimento
mexe na CPU do jogo, o que é diferente, mas o gargalo
pode continuar sendo a tradução de ~16.000 GXBegin por frame,
a fila IPC, os shaders e a Mali-G31.

Nenhum número de ganho de FPS é apresentado neste documento
sem medir no hardware real. Os dados do projeto original
não foram alterados para preservar sua reconstrução fiel.

## Direitos e distribuição

Os pacotes A35 do projeto reconstroem código de
Resident Evil 4 de GameCube. A licença dos scripts e
documentação **não concede direitos de distribuição sobre
código do jogo**, texturas, música ou executáveis protegidos.
Este artifact não inclui o disco do jogo, texturas,
áudio, saves nem memory cards. Utilize para testes
locais com arquivos e autorizações apropriados.
