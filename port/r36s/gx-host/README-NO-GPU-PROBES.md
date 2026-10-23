# R36S — teste de desempenho removendo SOMENTE sondas gráficas de diagnóstico

## Por que este experimento é o próximo passo

O arquivo `diagnostico_gx_profundo_darkos.txt` mediu 1177 quadros gráficos
apresentados durante a sessão de 180 segundos, sem erro de GL, shader ou IPC,
e encerramento com código 0.

Nas 13 janelas de maior carga (259 quadros e ~526 draw calls/quadro),
a decomposição média por quadro de `renderer.Run()` foi:

| Parte | Tempo médio |
|---|---:|
| Draw() | ~77,7 ms |
| Outros opcodes GX | ~58,9 ms |
| Finalização de cada chunk | ~30,8 ms |
| Upload dos VBO/IBO | ~6,5 ms |
| BindTexture | ~0,36 ms |

Os ~30,8 ms de **finalização** incluem uma coleta de timer GPU e
`FenceAfterChunk()`, esta última verificando até 8 fences de progresso
de uma sessão. Esses fences são **exclusivamente diagnósticos** (mede atraso
da GPU); não participam da renderização nem da proteção do stream
de uniforms.

## O que esta variante altera

Este patch adiciona a opção de ambiente `RE4_GX_GPU_PROBES=0`.
Quando habilitada, em `gx_gl.cpp`:

- pula `TimerInit()` e os queries GPU por tempo;
- pula `FenceAfterChunk()` — apenas o observador de progresso;
- **mantém** `glFinish()` antes de `SDL_GL_SwapWindow()` em modo 5;
- **mantém** `StreamNextPart()`, `streamFence[]` e `glClientWaitSync()`
  de proteção da uniform ring (não desativa sincronização de segurança);
- mantém todos os opcodes GX, draw calls, shaders, buffers de vértices
  e output do jogo inalterados.

Se reduzir o valor `CLEANUP_MS` e melhorar FPS sem defeitos gráficos,
podemos preparar uma versão estável específica para DarkOSRE posteriormente.
Sem validação visual e A/B, **não é solução comprovada**.

## Como instalar e testar

Baixe o artifact `CubeEvil4-R36S-Sem-GPU-Probes-ARM64` no GitHub Actions,
descompacte em `/roms/ports`, sem apagar `data`, `modules`, o coreproc,
as bibliotecas, o executável **estável** `r36s_game_boot` nem
`RE4-R36S-Estavel.sh`.

O ZIP contém somente:

- `cubeevil4_live_gx/r36s_game_deep_profile` atualizado;
- `RE4-R36S-Perfil-GX-Profundo.sh` com controle opt-in do recurso;
- **`RE4-R36S-Sem-Probes.sh`** para executar variante experimental;
- instruções.

No menu Ports, execute **`RE4-R36S-Sem-Probes.sh`**.
Entre no jogo e observe principalmente a cena pesada inicial. A duração
máxima é 180s, com saída segura SELECT+START por 2s (se SELECT mapear
para SDL BACK) ou ESC por teclado.

Relatório independente:

`/roms/ports/cubeevil4_live_gx/diagnostico_gx_sem_probes_darkos.txt`

Precisamos dos valores `PERFORMANCE_WINDOW_SECONDS`, `STAGE_TIMING_MS`,
`DEEP_GX_WINDOW`, `RESULT` e `EXIT_CODE`.
A comparação com a execução anterior do mesmo profiler, com as probes ativas,
permite ver se o overhead diagnosticado desapareceu.

### Avisos

**Nenhuma alteração no binário funcional** `r36s_game_boot`. O código
compilado no novo binário de experimentos tem uma opção condicional que
permanece ativada por padrão (probes normais) quando o launcher de baseline
`RE4-R36S-Perfil-GX-Profundo.sh` é utilizado.

A medição cronometra duração wall-clock de chamadas GL e pode incluir
bloqueio do driver. Desligar sondas não fornece temporizações próprias da
GPU; `gpuMicros` / `behindChecks` podem ficar em zero.

O teste pode ser mais rápido, igual ou mais lento: somente o dispositivo
físico pode comprovar. Se houver qualquer anomalia, voltar para o launcher
**`RE4-R36S-Estavel.sh`**.
