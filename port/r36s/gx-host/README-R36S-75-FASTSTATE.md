# Resident Evil 4 no R36S — GX rápido sem piorar a qualidade 75%

## Por que este patch existe

O usuário confirmou que prefere **qualidade 75% (480x396)**; as opções
320x264 e 240x198 são borradas demais para jogar. Os três logs do
console físico R36S DarkOSRE confirmaram 180 segundos sem erros GL,
shader, IPC ou crash:

| Tamanho interno | Quadros apresentados | Draws da sessão | FPS médio de janelas com >=400 draws/frame | Pico RSS frontend |
|---|---:|---:|---:|---:|
| **480x396** | **1893** | **404273** | **~6,0** | ~276 MiB |
| 320x264 | 2267 | 599741 | ~8,1 | ~269 MiB |
| 240x198 | 2431 | 637596 | ~8,3 | ~266 MiB |

Os intervalos de gameplay não são cenas idênticas; estas médias são
descritivas, **não um benchmark A/B rigorosamente controlado**. Em
480x396, `MemAvailable` mínimo nos registros foi ~380 MiB, sem swap.

A renderização de 480x396 ainda faz ~400–530 desenhos por quadro nas
partes pesadas. A qualidade visual deve ser mantida: dois canais
de iluminação dinâmica por shader, fog/DOF off (como no Ultra Seguro),
**nenhum corte de distância e nenhum objeto removido**.

## O que foi otimizado

Novo executável **`r36s_game_a35_faststate`**, compilado com
`-O3 -mcpu=cortex-a35 -mtune=cortex-a35 -flto=auto`.

**1. Ignorar estado GX BP inalterado:** o jogo envia snapshots de
256 registradores BP em chunks; antes, mesmo valores BP_STATE idênticos
marcavam shaders como sujos, levando a pesquisas repetidas de
`ShaderKey` e cópias de estado. Agora, sob
`RE4_R36S_STATE_DEDUP`, gravações BP sem alteração de valor saem
imediatamente. **Exceção essencial**: gravações de ação em BP 0x52
(EFB copy) e 0x5A (autofocus) **ainda executam** quando o valor é
igual, porque têm efeitos independentes da alteração do registrador.

**2. Ignorar XF idêntico:** repetição de palavras XF sem alteração
não recria matrizes, luzes ou cores, nem marca ranges como sujos.
Mudanças reais continuam sendo aplicadas normalmente. O streaming
de uniforms preserva fences e sincronização GPU.

**3. Eliminar clear de framebuffer redundante:** se o renderer já
tem XFB válido, `Present(0,0,kW,kH)` cobre a tela inteira com
`GL_BLEND` e `GL_SCISSOR_TEST` desativados (e faz clear próprio
se `VISetBlack` está ativo). O clear preto que o frontend fazia
imediatamente antes era desnecessário. **Antes do primeiro XFB**,
o clear preto continua sendo executado para preservar o boot.

Nenhuma chamada de `glFinish` foi retirada; modo gráfico 5 e
VSync1 continuam como na versão validada. O shader binary cache do
driver Mali também permanece em uso.

## A/B controlado e proteção do que já funciona

O pacote publica DOIS executáveis separados, compilados a partir
do mesmo código-base e com **as mesmas flags Cortex-A35**:

- `r36s_game_a35_baseline`: equivalente ao renderizador anterior,
  sem as otimizações deste patch (controle).
- `r36s_game_a35_faststate`: redução de BP/XF repetidos e clear
  de tela sem necessidade.

Os dois usam exatamente **480x396**, Ultra Seguro (2 luzes, fog/DOF
off, distância ilimitada), shader cache e `glFinish`/VSync1.
**Nenhum dos dois substitui `r36s_game_boot` ou o
`r36s_game_a35_scalable` já testado.**

A versão rápida informa a cada 5s:

`FASTSTATE_WINDOW FRAMES=... BP_EQUAL_SKIPPED=...
XF_EQUAL_SKIPPED=... BP_ACTION_PRESERVED=... OUTPUT_CLEARS_SKIPPED=...`

E ao encerrar `FASTSTATE_FINAL`.
Se não houver redução mensurável em quadros ou as cenas piorarem,
**não adotar a otimização**. Otimização é hipótese até medir
no R36S real.

## Instalação e procedimento

Artifact: `CubeEvil4-R36S-75pct-GX-State-Otimizado-ARM64`.
Extraia ZIP em `/roms/ports`. O pacote contém:

- `cubeevil4_live_gx/r36s_game_a35_baseline`
- `cubeevil4_live_gx/r36s_game_a35_faststate`
- `RE4-R36S-75-Referencia.sh`
- `RE4-R36S-75-Otimizado.sh`
- `RE4-R36S-75-Teste-Estado.sh`
- `LEIA-ME-75-STATE-FAST.md`

**Não apague** o diretório `data/`, módulos, libraries,
`re4_coreproc`, `r36s_game_boot`,
`RE4-R36S-Estavel.sh`, `r36s_game_a35_scalable`, ou o
memory card `data/memcard/card-a.bin`. Faça backup deste último
antes de testar.

1. Execute `RE4-R36S-75-Referencia.sh`, entre na primeira área
   e siga um trajeto de referência. Tempo máximo: 180 segundos.
2. Execute `RE4-R36S-75-Otimizado.sh` e tente repetir o mesmo
   trajeto/câmera/tempo. Verifique personagens, HUD, sombras, fogo,
   água e transições de tela para possíveis defeitos.
3. Envie os dois arquivos:

`cubeevil4_live_gx/diagnostico_a35_75_baseline_darkos.txt`
`cubeevil4_live_gx/diagnostico_a35_75_faststate_darkos.txt`

Se surgir problema de gráfico ou travamento, mantenha apenas
`RE4-R36S-Estavel.sh` ou `RE4-R36S-Qualidade-75.sh`.

### Sobre árvores e objetos "inúteis"

**Não foram apagados**. O stream GX recebido por `Draw`
tem geometria, texturas, matrizes e registradores, mas **não**
identifica qual draw representa uma árvore, Leon, uma sombra
ou um item da missão. O Ultra antigo descartou mais de 235 mil
draw calls por distância e apresentou objetos pretos; não
repetiremos cortes genéricos. Remoção segura exige descobrir
os IDs de recursos/instâncias no *core* e separar elementos de
cenário daqueles com interação, colisão, efeitos e oclusão.
Após isso podemos fazer uma lista controlada por sala/objeto,
sem afetar gameplay ou salvar dados modificados.

A recompilação `-O3` Cortex-A35 **só otimiza frontend/renderer**;
`re4_coreproc` e demais bibliotecas seguem como estavam.
