# Resident Evil 4 R36S — estudo de qualidade 75%, 50% e 37,5% com monitor de RAM

## Resultado comprovado do A/B anterior

O usuário enviou logs de **180 segundos** com a mesma CPU otimizada
(`-O3 -mcpu=cortex-a35 -mtune=cortex-a35 -flto=auto`) e o mesmo preset
Ultra Seguro (duas luzes, DOF/fog off, sem culling por distância).

| Métrica | EFB 640x528 | EFB 320x264 |
|---|---:|---:|
| Quadros gráficos apresentados em 180 s | 1291 | 2790 |
| Trecho com ~513 draws/quadro e ~125k triângulos/quadro | ~4,25 FPS | ~5,86 FPS |
| FPS em tela/menu já carregado | ~9 | ~30 |
| Draw calls realmente descartados | 0 | 0 |
| Erros GL, shader, IPC | 0 | 0 |
| Encerramento | código 0 | código 0 |
| Pico de RSS registrado no frontend | ~215 MiB | **~429 MiB** |
| RSS final do frontend | ~211 MiB | ~167 MiB |
| RSS final do núcleo | ~116 MiB | ~122 MiB |

Em janelas de gameplay de carga aproximadamente semelhante, a meia
resolução teve ~**38% mais quadros/s** (5,86 vs. 4,25). É uma análise
comparativa **indicativa**, pois o usuário percorreu cenas diferentes
e os shaders e uploads de recursos também variam.

O modo de meia resolução atingiu cerca de 11–13 FPS em trechos posteriores
com ~365–450 draws/quadro, mas não devemos comparar diretamente com
trechos mais pesados em 640x528.

A métrica de sessão inteira não é um FPS médio representativo de jogo:
inclui menus, carregamentos e períodos sem gráficos complexos.

### Por que medir memória do sistema?

Na meia resolução, o `VmRSS` do frontend subiu para ~429 MiB em
momentos de gameplay, depois voltou para ~167 MiB ao final. O
`VmHWM` ficou em ~438 MiB; o framebuffer menor por si só não explica
uma diferença tão grande contra os ~215 MiB de RSS registrados no teste
de resolução cheia. **Não há prova da causa**; o jogo progrediu mais,
criou novos recursos e percorreu trechos diferentes.

O R36S vê ~895 MiB no Linux, sem swap. A memória GPU pode ser
compartilhada e `VmRSS` não representa memória real exclusiva.
O novo binário registra `SYSTEM_MEMORY_SAMPLE STAGE=PERFORMANCE_WINDOW
MEM_AVAILABLE_KB=...` a cada 5 segundos, além de `VmRSS`/`VmHWM` dos
dois processos. Se `MemAvailable < 128 MiB`, imprime um aviso,
**sem matar o jogo nem interferir no save**.

## Novos presets experimentais

Um novo binário **`r36s_game_a35_scalable`** usa EFB interna fixa:

| Script | EFB real | Porcentagem dos pixels originais |
|---|---:|---:|
| `RE4-R36S-Qualidade-75.sh` | **480x396** | 56,25% |
| `RE4-R36S-Qualidade-50.sh` | **320x264** | 25% |
| `RE4-R36S-Qualidade-37.sh` | **240x198** | ~14,06% |

O nome refere-se à porcentagem da largura/altura, não da contagem
de pixels. **A qualidade 37,5% é muito baixa** e pode causar serrilhado,
sombras quebradas, HUD borrado ou erros de cópia EFB. Não assuma
ganho proporcional em FPS. A escala não muda durante o frame:
seleciona-se uma resolução **somente na inicialização**, por segurança.

O binário separado `r36s_game_a35_scalable` não altera nem substitui:

- `r36s_game_a35_half` já testado;
- `r36s_game_a35_full` já testado;
- `r36s_game_boot`, `RE4-R36S-Estavel.sh`;
- `re4_coreproc`, bibliotecas/módulos, dados originais e saves.

Flags Cortex-A35 e Shader Binary Cache GLES3 estão mantidas.
O modo gráfico continua em VSync1 e `glFinish` antes do swap
(SDL2 KMSDRM), pois esse método evitou o travamento no frame 16.

O renderizador **não remove árvores/plantas aleatoriamente**:
os draws GX não carregam nomes de modelos. Antes, uma tentativa de
culling genérico eliminou 235.546 chamadas de desenho e deixou objetos
pretos. Remover vegetação requer recursos e modelos identificados no
núcleo de jogo, não uma decisão baseada apenas na cor/transparência.

## Como instalar e testar

Artifact `CubeEvil4-R36S-Escalas-Qualidade-A35-ARM64` da action
`R36S - A35 EFB quality scales and RAM telemetry`.
Extraia o ZIP para `/roms/ports` preservando todos os arquivos.

Primeiro execute `RE4-R36S-Qualidade-75.sh`, teste a primeira área por
180 segundos e confira se imagem e HUD estão corretos. Depois execute
`RE4-R36S-Qualidade-50.sh` (repete o teste com instrumentação de RAM).
Se a experiência visual permitir, execute **`RE4-R36S-Qualidade-37.sh`**;
esse último é um experimento agressivo que pode ter visual inaceitável.

Cada launcher produz um TXT exclusivo:

- `cubeevil4_live_gx/diagnostico_escala_480x396_a35_darkos.txt`
- `cubeevil4_live_gx/diagnostico_escala_320x264_a35_darkos.txt`
- `cubeevil4_live_gx/diagnostico_escala_240x198_a35_darkos.txt`

Procure `R36S_EFB_RENDER_SCALE` e `EFB_INTERNAL_SIZE`, depois
`PERFORMANCE_WINDOW_SECONDS`/`FPS_PRESENTED`,
`FRONTEND_VmRSS`/`VmHWM`, `SYSTEM_MEMORY_SAMPLE`, `GL_ERRORS`,
`LOW_SPEC_CULLED_DRAWS=0`, e `EXIT_CODE=0`.

Faça um backup do memory card virtual `data/memcard/card-a.bin` antes
do teste, porque os logs de estabilidade não são prova de segurança
de saves em todas as situações. O R36S pode sair com SELECT+START
pressionados por 2 segundos (se SELECT mapear para SDL BACK); o tempo
máximo normal é 180 segundos.

Em caso de erro, use **`RE4-R36S-Metade-Resolucao.sh`** (que já
terminou uma sessão com exit 0) ou **`RE4-R36S-Estavel.sh`**
para máxima compatibilidade visual.
