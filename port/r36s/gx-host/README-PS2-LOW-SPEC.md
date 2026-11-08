# CubeEvil4 R36S / DarkOSRE — gráficos Low Spec inspirados no RE4 do PS2

## Correção após teste real — Ultra anterior com objetos pretos

O usuário testou `RE4-R36S-PS2-Balanceado.sh` e o antigo
`RE4-R36S-PS2-Ultra.sh` no R36S com **180 segundos**, ambos encerrando
normalmente sem erros reportados pelo OpenGL. Porém o **Ultra apresentou
graves defeitos visuais: muitos itens ficaram completamente pretos**.

Os logs confirmaram:

- **Balanceado:** 1268 quadros, 251377 draws executados, **0 culls**,
  401 passos de DOF ignorados, 118 shaders, 0 erros, código 0;
- **Ultra anterior:** 1606 quadros, 173970 draws executados e
  **235546 draws descartados por heurística de distância** — mais de
  metade das chamadas consideradas/executadas. 624 passos DOF
  ignorados, 63 shaders, 0 erros, código 0.

A falta de erros GL significa apenas que o driver aceitou os comandos:
**não indica visual correto**. A iluminação zerada também podia deixar
materiais pretos porque alguns usam luz dinâmica com ambiente escuro.

### Correções desta V2

- **Culling de objetos distantes desativado em todos os perfis**:
  a heurística antiga não tinha escala de cena comprovada e pode retirar
  geometria da iluminação/sombra. O renderizador **recusa a ativação
  sem `RE4_R36S_UNSAFE_FAR_CULL=1`**; o novo launcher não habilita esse modo.
- **Nunca usar zero luzes dinâmicas:** o renderizador força duas se
  receber 0; o **Ultra Seguro usa 2**. O PS2 Leve usa uma luz dinâmica
  experimental e deve ser verificado visualmente.
- **Ultra Seguro** remove DOF e fog, mas mantém todas as geometrias e
  duas luzes dinâmicas. É uma variante de efeitos reduzidos, **não**
  garantimos o grande ganho de FPS do Ultra anterior que eliminava
  metade da cena.
- Log novo `diagnostico_ps2_ultra_seguro_darkos.txt` preserva
  `diagnostico_ps2_ultra_seguro_darkos.txt` do teste problemático.

A versão estável `RE4-R36S-Estavel.sh` e a variante Balanceado
permanecem os pontos de comparação. **Não há LOD automático igual ao
port nativo de PS2**, pois exigiríamos simplificar modelos ou mexer
na lógica/representação da cena, não cortar draws por distância arbitrária.

---

## Objetivo

O console **já executa gameplay real** sem falhas visuais conhecidas,
mas a primeira área pesada apresenta aproximadamente **3,6–4,2 quadros
processados por segundo**. Reduzir efeitos e distância de desenho pode
ajudar sem depender exclusivamente de cache de shaders. Este é um
**experimento de desempenho, NÃO uma versão otimizada validada**.

O port original para PlayStation 2 sacrificou resolução e detalhes dos
modelos, efeitos de luz e neblina, reflexos/água e distância visível.
**Aqui não possuímos os modelos LOD nem o código de cena PS2**, portanto
não aplicamos automaticamente a mesma geometria; simplificamos os
caminhos gráficos GX disponíveis nesta implementação.

## Perfil gráfico (selecionável no menu Ports)

| Script | Luzes dinâmicas por canal | DOF/desfoque | Fog/neblina | Cortar objetos distantes |
|---|---:|---|---|---|
| `RE4-R36S-PS2-Balanceado.sh` | até 2 | desativado | mantida | NÃO |
| `RE4-R36S-PS2-Leve.sh` | até 1 | desativado | mantida | NÃO |
| `RE4-R36S-PS2-Ultra.sh` (**corrigido**) | até 2 | desativado | removida | NÃO |
| `RE4-R36S-Estavel.sh` | original | original | original | NÃO (versão anterior preservada) |

A remoção de desenho por distância foi **desligada** porque o
culling antigo removia indiscriminadamente até superfícies necessárias
para a iluminação. Não reutilize o ZIP V1 do Ultra para jogar.

O modo Balanceado é o teste inicial recomendado. Os outros podem
ocasionar *popping* de cenário, iluminação excessivamente escura ou
efeitos desaparecendo. Se a imagem estiver errada, volte ao estável.

## Mudanças técnicas comprováveis no código

1. Em `gx_gl.cpp`, a geração de `ShaderKey` mantém iluminação de
   material/ambiente mas reduz a quantidade de luzes dinâmicas calculadas
   por vértice. A GPU executa shaders potencialmente mais simples. Os
   shaders compilados são diferentes e usados somente na versão Low Spec.
2. Em BP `0x5A`, ignora a etapa de *autofocus* que gera e executa
   múltiplas passagens de desfoque de profundidade; o resto dos comandos
   GX continua em ordem.
3. Na variante Ultra, o shader fragment deixa de aplicar neblina GX;
   isso altera a estética e não necessariamente reduz muitos draws.
4. **Não corta draw calls por distância** nos presets corrigidos.
   A heurística de distância anterior permanece somente para
   depuração com flag explícita `RE4_R36S_UNSAFE_FAR_CULL=1`,
   nunca definida pelos scripts incluídos.
5. Mantém o cache persistente existente de programas GLES3 da Mali,
   com `GL_PROGRAM_BINARY_RETRIEVABLE_HINT`/`glGetProgramBinary`,
   `glProgramBinary` e fallback de compilação se necessário.

**Não tocamos** no `glFinish()` antes do `SDL_GL_SwapWindow()`, porque
foi ele que evitou a queda no quadro 16. VSync permanece solicitado
em 1, BaseVertex desligado, sem modificações no processo `re4_coreproc`.

## Instalação

Baixe o artifact `CubeEvil4-R36S-PS2-LowSpec-Corrigido-ARM64` do GitHub Actions
da branch `chatgpt/r36s-ps2-safe-ultra-fix`. Extraia o ZIP em
`/roms/ports`. Ele **adiciona**:

- `cubeevil4_live_gx/r36s_game_ps2`
- `RE4-R36S-PS2-LowSpec.sh`
- `RE4-R36S-PS2-Balanceado.sh`
- `RE4-R36S-PS2-Leve.sh`
- `RE4-R36S-PS2-Ultra.sh`
- `LEIA-ME-PS2-LOW-SPEC.md`

Preserve **todos os seus arquivos existentes**, especialmente
`r36s_game_boot`, `RE4-R36S-Estavel.sh`, `re4_coreproc`,
`modules/`, `data/` e `data/memcard/card-a.bin`. Recomenda-se
backup do cartão de memória virtual antes dos testes.

## Logs e como comparar

Primeiro execute `RE4-R36S-PS2-Balanceado.sh`, entre na primeira
área e teste por 180 s (ou saia com SELECT+START por 2 s, se SELECT
mapear para SDL BACK). Depois opcionalmente teste o perfil PS2 Leve,
e por último Ultra se for necessário.

Cada perfil tem seu próprio relatório:

- `/roms/ports/cubeevil4_live_gx/diagnostico_ps2_balanceado_darkos.txt`
- `/roms/ports/cubeevil4_live_gx/diagnostico_ps2_ps2_darkos.txt`
- `/roms/ports/cubeevil4_live_gx/diagnostico_ps2_ultra_darkos.txt`

Procure `LOW_SPEC_ENABLED`, `R36S_LOWSPEC`,
`LOW_SPEC_WINDOW`, `LOW_SPEC_CULLED_DRAWS`,
`DOF_PASSES_SKIPPED`, `FPS_PRESENTED`, `SHADERS`, 
`SHADER_CACHE_FINAL_HITS`, `GL_ERRORS` e `EXIT_CODE`.
Queremos um aumento de FPS medido **na mesma área e com o mesmo
trajeto**, sem defeitos visuais que prejudiquem a jogabilidade.

A pasta do cache é compartilhada entre os experimentos em
`cubeevil4_live_gx/cache/shader_binaries/`. Novos shaders simplificados
têm chaves derivadas do texto de seus fontes; por isso podem criar novos
arquivos, sem apagar os anteriores. No log de primeira execução
`SHADER_CACHE_FINAL_HITS` não precisa ser zero: o núcleo pode emitir
chunks `RESET` e os programas reaproveitam o cache gravado segundos
antes nessa mesma execução. No primeiro teste anterior foram gerados
99 arquivos (aproximadamente 1,8 MiB), com 29 hits e sem erros.

## Limitações importantes

- **Sem áudio SDL2 conectado** no frontend atual.
- A GPU é uma Mali-G31; a RAM livre não resolve gargalos de Draw() ou
  compilação de shaders que já estão em cache.
- Cache de shaders persistente reduz carregamento/travadinhas; não deve
  elevar FPS estável em cenas já renderizadas.
- Nem todos os modelos usam uma matriz de posição única. O culling
  conservador ignora os outros e pode reduzir poucos draws. Se o
  contador `CULLED_DRAWS=0`, isso não representa falha do patch: apenas
  ausência de geometrias que cumpram o filtro.
- Desativar DOF e iluminação muda a estética; corte de geometria pode
  fazer elementos aparecerem repentinamente. **Esta não é uma reprodução
  literal da versão PS2 da Capcom**.
- O processo do jogo e o renderer compartilham RAM da máquina;
  sem swap, evite lotar memória com caches sem limites.
