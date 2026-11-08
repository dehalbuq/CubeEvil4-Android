# R36S DarkOSRE — sessão GLES sincronizada (experimental)

## O que já foi observado no hardware

O Resident Evil 4 GameCube G4BE08 já executou um teste de 45 segundos com
imagem visível e troca SDL2/KMSDRM, 487 quadros, 119.788 triângulos,
7 shaders, sem erros GL/IPC e encerramento código 0.

A configuração funcional era chamada modo 1: `glFinish()` antes de
`SDL_GL_SwapWindow`. O pedido de VSync=0 retornou `ACTUAL=1`; logo
**desligar o VSync não foi confirmado** e provavelmente não explica
a melhora. Reproduzimos aqui a configuração mais conservadora:

- **modo 5:** `glFinish()` antes de cada swap, intervalo solicitado **1**
- O modo 1 e os demais modos de diagnóstico continuam disponíveis.
- O experimento não altera `re4_coreproc`, as bibliotecas nem os módulos.

Isso é um **candidato a estabilidade**, não correção comprovada para todos
os cenários. `glFinish()` pode reduzir FPS; vamos medir.

## Arquivos do pacote

O artifact `CubeEvil4-R36S-GLES-Estavel-ARM64` inclui:

- `cubeevil4_live_gx/r36s_game_boot` (executável atualizado)
- `RE4-Primeiro-Boot.sh` (modo 5, 180 segundos por padrão)
- `RE4-R36S-Estavel.sh` (3 minutos, modo 5)
- `RE4-R36S-Teste-10min.sh` (10 minutos, modo 5)
- `LEIA-ME-ESTAVEL.md`

Basta extrair para `/roms/ports` e substituir esses arquivos.
**Não apague:** `cubeevil4_live_gx/data/`, `modules/`,
`re4_coreproc`, `libre4_main.so`, `libre4_platform.so` ou
o memory card `data/memcard/card-a.bin`. Faça backup do memory card
antes do teste prolongado.

## Como sair com segurança

No R36S, **segure SELECT + START juntos por pelo menos 2 segundos**.
Essa combinação usa `SDL_CONTROLLER_BUTTON_BACK` e
`SDL_CONTROLLER_BUTTON_START`; se SELECT não mapear para BACK,
a combinação pode não funcionar. Com teclado, pressione **ESC**.
Se não funcionar, a sessão tem duração máxima configurada (180/600 s).
Não retire o cartão nem desligue o console durante gravação do save.

O teste não precisa de root e **ainda não reproduz áudio**: o fluxo de
áudio do núcleo é consumido mas não é enviado aos alto-falantes.

## Diagnósticos e interpretação

Arquivos em `/roms/ports/cubeevil4_live_gx/`:

- `diagnostico_primeiro_boot_darkos.txt`: última execução
- `diagnostico_primeiro_boot_modo5_darkos.txt`: cópia do modo estável

Procure:

- `GPU_SWAP_DIAG_MODE=5` e `GPU_MODE_DESC=vsync1_glFinish_before_swap_R36S_candidate`
- `SDL_SWAP_INTERVAL_REQUESTED=1`, `ACTUAL=1`
- `PERFORMANCE_WINDOW_SECONDS`, `FPS_PRESENTED`: taxa média de
  **quadros recebidos e processados na janela de 5 segundos**.
  Não é uma medição de 60 FPS de gameplay ou de fluidez percebida.
- `FRONTEND_VmRSS`, `CORE_PID_<pid>_VmRSS`: RSS por processo.
  Memória compartilhada pode ser contada em ambos; a soma não é RAM real
  total do sistema.
- `SESSION_STOP_REASON=time_or_frame_limit` e `EXIT_CODE=0`:
  sessão terminou pelo limite de tempo e sem falhas detectadas.
- `RESULT=USER_EXIT_CLEAN`: saída solicitada pelo usuário.
- `CRASH_SIGNAL` / `CRASH_STAGE`: falha detectada pelo rastreador.

Se o modo 5 apresentar regressão, continue com o launcher anterior
`RE4-GPU-Sem-VSync.sh` (modo 1) preservado da versão anterior,
e envie **os dois logs**. Isso distingue o pedido explícito de VSync do
efeito de `glFinish()`.

O alvo imediato é **confirmar menu e controle e uma sessão mais longa**;
áudio SDL2, opções, save durante gameplay e otimização serão etapas
posteriores. Não distribuir imagens do jogo, ISOs ou arquivos extraídos.
