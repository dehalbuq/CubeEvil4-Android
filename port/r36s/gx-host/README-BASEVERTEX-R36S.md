# R36S / Mali-G31 — experimento BaseVertex para aumentar FPS

## Evidência já coletada

A versão estável (modo 5, `glFinish` antes de cada swap) rodou 180 segundos com
Resident Evil 4 G4BE08, sem erros. No perfil gráfico, o jogo real apresentou
**1.268 quadros**, **202.780 draws**, ~**32 milhões de triângulos** e
nenhum erro GL/shader/IPC, encerrando com código 0.

O profiler identificou a diferença entre menu e jogo real:

| Cena | GX_RUN (média) | glFinish | Swap | FPS aproximado |
|---|---:|---:|---:|---:|
| Menu | ~13 ms | ~94 ms | ~0,33 ms | ~9 |
| Gameplay pesado | ~197 ms | ~32 ms | ~0,32 ms | ~3,5 |

A duração de `renderer.Run` **não é tempo puro da CPU**; pode incluir
esperas dentro do driver. Ela é a principal etapa medida, e é nela que
pretendemos reduzir o número de chamadas de configuração de atributos.

## Por que testar BaseVertex?

O código do renderizador GX já tem suporte a
`glDrawElementsBaseVertex` (GLES 3.2, OES/EXT) que evita reconfigurar
`glVertexAttribPointer` em cada draw que muda o deslocamento dos
vértices. Entretanto o launcher do R36S força `RE4_GX_BASEVERTEX=0`,
impedindo seu uso. O driver Mali-G31 informou GLES 3.2 e
`GL_OES_draw_elements_base_vertex` / `GL_EXT_draw_elements_base_vertex`.

Nesta versão **opcional**, o launcher experimental ativa essa função
**somente se estiver exposta** pelo driver. O programa imprime
`GX_BASEVERTEX_ACTIVE=1` se a função for realmente localizada.
Se mostrar `0`, o teste não ativou a otimização e os tempos devem ser
comparados como baseline.

**Atenção:** é uma alternativa experimental para este driver. Não sabemos
se os vértices e índices do Resident Evil 4 renderizam com precisão
quando BaseVertex é usado no R36S. Qualquer defeito gráfico deve ser
relatado; não tornar a opção padrão antes de verificá-la.

## Como instalar/testar

Baixe o artifact `CubeEvil4-R36S-BaseVertex-Experimento-ARM64`, extraia
em `/roms/ports`, preservando `cubeevil4_live_gx/data`,
`modules`, `re4_coreproc` e **`r36s_game_boot`** (versão estável).
Este pacote substitui apenas `r36s_game_profile` (perfil de teste),
o launcher de perfil e adiciona um novo launcher experimental.

Execute pelo menu Ports:

**`RE4-R36S-Teste-BaseVertex.sh`**

Entre na primeira área e observe se há artefatos, sombras incorretas ou
texturas deslocadas. O script encerra após 180 segundos, ou você pode
segurar SELECT+START por dois segundos para sair (se o botão SELECT mapear
para SDL BACK). O relatório independente será:

`/roms/ports/cubeevil4_live_gx/diagnostico_basevertex_darkos.txt`

Verifique os campos:

- `GX_BASEVERTEX_ACTIVE` — função realmente ativada (1) ou não (0);
- `STAGE_TIMING_MS` — GX_RUN, glFinish, Present e EGL swap por janela;
- `DRIVER_WORK_WINDOW` — número de draws e setups de atributos por janela;
- `FPS_PRESENTED` — throughput de chunks apresentados, não FPS de
  animação do jogo medido internamente;
- `CRASH_STAGE`, `SHADER_ERRORS`, `GL_ERRORS` — eventuais regressões.

O teste gráfico estável continua em `RE4-R36S-Estavel.sh`.
Não alteramos os arquivos do jogo, drivers ou o processo do núcleo.
