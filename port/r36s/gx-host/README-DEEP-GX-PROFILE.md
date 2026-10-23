# R36S - análise interna do renderer.Run() (GPU/C++)

## Resultado da comparação já feita

Em 180 s, BaseVertex ficou ativo (`GX_BASEVERTEX_ACTIVE=1`), sem
erros de OpenGL, shaders ou IPC e 1627 quadros apresentados.
Na primeira área com ~500 draws/frame:

- Base sem BaseVertex: ~3,93 quadros/s, `GX_RUN_AVG` ponderado ~173 ms
  (12 janelas contendo 242 quadros).
- Com BaseVertex: ~3,75 quadros/s, `GX_RUN_AVG` ponderado ~179 ms
  (9 janelas contendo 173 quadros).
- Condições não são cenas idênticas; não demonstram ganho. Por segurança,
  continuar mantendo `RE4_GX_BASEVERTEX=0` no executável estável.

O caminho BaseVertex ativo mostrou ~0,033 a 0,036 configurações de
atributos por draw na primeira sala, o que comprova poucos setups,
mas não é uma medida A/B desse contador.

## Novo binário diagnóstico opcional

`r36s_game_deep_profile`, completamente separado de `r36s_game_boot`
(versão estável) e `r36s_game_profile` (profiling anterior).

A cada 5 segundos imprime `DEEP_GX_WINDOW` com:

- `UPLOAD_MS`: ligação e atualização de VBO/IBO do chunk com glBufferData;
- `DISPATCH_MS`: varredura dos opcodes GX e operações relacionadas;
- `DRAW_MS`: tempo acumulado nas chamadas de `Draw` (shader, estado,
  uniforms, texturas, glDrawElements);
- `TEX_MS`: tempo em `BindTexture` nos comandos RE4_GX_OP_TEXTURE;
- `OTHER_OPS_MS`: resto do tempo da varredura GX;
- `CLEANUP_MS`: timers, fences e limpeza do estado GL ao fim do chunk;
- `DRAW_OPS` e `DRAW_MS_PER_OP`: quantidade e tempo médio dos draw calls.

Tempos medidos por `std::chrono::steady_clock` incluem espera no driver,
não são clocks de execução da GPU. O diagnóstico introduz algum overhead,
não usar como versão normal de gameplay.

## Instalação e teste

Baixe o artifact `CubeEvil4-R36S-GX-Deep-Profiler-ARM64` e descompacte em
`/roms/ports`, preservando todos os outros binários, dados extraídos,
bibliotecas, módulos e memória virtual do jogo.

Pelo menu Ports, execute **`RE4-R36S-Perfil-GX-Profundo.sh`**, entre na
primeira área jogável e continue até encerrar a sessão de três minutos.
Saída antecipada: SELECT+START por dois segundos (se SELECT mapear para
SDL BACK), ou ESC no teclado.

Relatório separado:

`/roms/ports/cubeevil4_live_gx/diagnostico_gx_profundo_darkos.txt`

Uma segunda variante de comparação pode ser executada manualmente com
`RE4_R36S_BASEVERTEX=1 sh RE4-R36S-Perfil-GX-Profundo.sh`, gerando
`diagnostico_gx_profundo_basevertex_darkos.txt`; primeiro use o padrão 0.

O teste mantém modo gráfico 5 (glFinish antes de SwapWindow, VSync1) e
**não altera a versão estável, bibliotecas, núcleo, ISO ou conteúdo do jogo**.
