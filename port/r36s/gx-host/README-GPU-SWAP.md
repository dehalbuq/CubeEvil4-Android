# R36S Mali-G31: investigar falha no egl_swap do primeiro boot

## Evidência no hardware
O primeiro boot com os dados originais G4BE08 carrega o núcleo ARM64,
cria memory card, compila shaders e apresenta quadros reais. O relatório
instrumentado registrou SIGSEGV no quadro 16 durante SDL_GL_SwapWindow:
CRASH_STAGE=egl_swap, CRASH_PC=0x7f8a2b7d40,
CRASH_FAULT_ADDRESS=0x000003e800000619.

O PC pertence à região executável da libc do DarkOSRE (deslocamento
aproximado 0x87d40); **não** podemos afirmar que o bug é dentro do driver
Mali. Uma falha na sincronização, dados corrompidos ou estado EGL inválido
pode aparecer ali. Este patch **não corrige uma causa ainda desconhecida**:
ele permite isolar opções de apresentação e gestão de UBO.

## Novos modos de teste

Todos partem do mesmo executável `r36s_game_boot`; o launcher informa
`RE4_R36S_SWAP_MODE` e preserva cópia do relatório em
`diagnostico_primeiro_boot_modoN_darkos.txt`.

| Modo | Swap | Intervalo | Uniforms | Alteração |
|---|---|---|---|---|
| 0 | Sim | 1 | Stream persistente original | Baseline, pode repetir queda |
| 1 | Sim | 0 | Stream persistente original | `glFinish` antes da troca, sem vsync |
| 2 | Sim | 1 | Stream **não persistente** | `RE4_GX_PERSISTENT=0` |
| 3 | Sim | 1 | UBO legado | `RE4_GX_UNIFORM_STREAM=0` |
| 4 | Não | 0 | Stream persistente original | `glFinish` sem `SDL_GL_SwapWindow` (tela não atualiza) |

**Ordem sugerida**: primeiro modo 2, porque ele preserva vsync e swap,
mas evita o mapeamento persistente de 16 MiB que foi confirmado no log
anterior; se falhar, modo 3, depois 1 e finalmente 4 para comparação.
Não execute modos diferentes automaticamente em série: preserve e envie
o log de cada execução antes de tentar outro.

## Instalação (não apagar o pacote anterior)

Extraia o artifact `CubeEvil4-R36S-GPU-Swap-Isolamento-ARM64` diretamente
em `/roms/ports`, substituindo somente o frontend de primeiro boot e
os arquivos de inicialização. Preserve `re4_coreproc`,
`libre4_main.so`, `libre4_platform.so`, módulos, data e memory card.

- `RE4-Primeiro-Boot.sh` inicia o **modo 2** por padrão.
- `RE4-GPU-UBO-Antigo.sh` testa o modo 3.
- `RE4-GPU-Sem-VSync.sh` testa o modo 1.
- `RE4-GPU-Sem-Swap.sh` testa o modo 4, sem exibir quadros.

Todos criam/atualizam
`/roms/ports/cubeevil4_live_gx/diagnostico_primeiro_boot_darkos.txt`
e cópias por modo. Compartilhe **somente** o arquivo TXT, sem mídia
original do jogo.

## Limitações

Este é um teste de compatibilidade de driver/buffers. Mesmo 45 segundos
sem SIGSEGV não demonstram que Resident Evil 4 está jogável.
Os dados do processo **core** e frontend ficam em memórias separadas;
o VmRSS do frontend não representa o uso total do jogo.
O som continua não conectado a uma saída SDL2 de áudio.
