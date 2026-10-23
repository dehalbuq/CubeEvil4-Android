# Resident Evil 4 no R36S: LOD offline de polígonos (PROTÓTIPO, NÃO INSTALAR NO PORT)

## Conclusão dos últimos testes A/B a 480×396

O R36S já roda o jogo nativo em GLES3.2 com dois processos, shader binary cache,
`glFinish` obrigatório, presets PS2 seguros e CPU Cortex-A35 / `-O3 -flto`.
O usuário prefere a resolução interna **480×396** à definição inferior de
320×264 / 240×198.

Nos últimos dois logs reais de 180s:

| | Baseline | Faststate BP/XF |
|---|---:|---:|
| Quadros processados | 1872 | 1873 |
| Draws | 419771 | 417509 |
| Triângulos | 70.35 milhões | 68.77 milhões |
| Erros GL/IPC/shader / exit | zero / 0 | zero / 0 |
| XF sem mudança eliminados | - | 4241440 |
| Clears redundantes evitados | - | 1873 |

Isso confirma que a remoção de atualizações XF repetidas NÃO melhorou
apreciavelmente o FPS total, apesar de reduzir trabalho do código.
Os números de triângulos/draws variam com as cenas percorridas;
o faststate **não simplificou modelos**.

Em cenas pesadas registramos ~400–530 draw calls e até ~130 mil triângulos
por quadro, com 2–3 MiB de vértices sendo enviados por quadro.

## O que pode ser simplificado sem destruir modelos

O repositório já possui `tools/motion/meshbin.py`, um parser e serializador
do formato original ModelData .bin de GameCube, que entende posições,
normais, UV, cores, materiais, listas GX (quads, strips, triângulos),
partes e influências de esqueleto.

Este patch adiciona **`tools/motion/r36s_model_lod.py`**, uma
ferramenta Python **offline** para analisar e experimentar LODs
mais leves em arquivos .bin isolados **de propriedade do usuário**.
Não substitui `r36s_game_boot`, o `re4_coreproc`, módulos,
arquivos de disco ou memory card.

O algoritmo conservador:

- **recusa** malhas com múltiplas partes esqueléticas, múltiplos pesos
  ou morph targets e não tenta reescrever personagens;
- **não altera** bordas da malha, posições compartilhadas entre materiais,
  posições que compartilham UV/normais conflitantes;
- apenas agrupa posições internas, no mesmo índice de matriz óssea,
  quando próximas dentro de um percentual configurável do tamanho do
  modelo e com normais de orientação semelhante;
- remove triângulos que virariam degenerados após colapsar
  posições internas;
- evita inverter orientação de faces e preserva os índices originais
  de textura/normal/cor (a iluminação ainda pode mudar um pouco);
- converte GX strips/quads em listas de triângulos **somente** quando
  o novo display list também fica menor (para não aumentar custo
  de CPU/GPU ao expandir strips);
- preserva o **comprimento binário integral do .bin e os tamanhos
  de cada display list**, preenchendo o espaço não usado com GX NOP,
  para não deslocar offsets dentro do modelo;
- valida o novo arquivo com o parser GameCube, compara a contagem
  real de triângulos antes/depois e **nunca sobrescreve o original**.

Embora proteja bordas e UVs, é um algoritmo geométrico aproximado,
**não um decimador QEM profissional**. Ainda pode deformar superfícies
e não sabemos quantos triângulos dos modelos reais do RE4 serão elegíveis.
Nunca trate um arquivo gerado como seguro para gameplay sem validar visualmente.

## Como utilizar no Windows (sem instalar nada no R36S)

1. Instale **Python 3** com o comando `py --version` funcionando.
2. Baixe o artifact `CubeEvil4-R36S-LOD-Offline-Ferramenta` do Actions e
   extraia na pasta `C:\\R36S-LOD` (vai criar `r36s-lod/tools`).
3. Copie **um arquivo da sua própria extração GameCube**, por exemplo
   `/roms/ports/cubeevil4_live_gx/data/disc0/st1/r100.das`
   (no cartão SD do R36S) para `C:\\R36S-LOD\\r100.das`.
   O caminho `st1` pode ter letras maiúsculas ou minúsculas.
4. No PowerShell:

```powershell
cd C:\\R36S-LOD
py r36s-lod\\tools\\r36s_extract_bin.py r100.das --list
py r36s-lod\\tools\\r36s_extract_bin.py r100.das --out modelos
```

O primeiro comando apenas lista os ModelData válidos dentro do .das.
O segundo **copia** cada modelo BIN (não altera o .das original) para
`C:\\R36S-LOD\\modelos` e informa triângulos, partes e materiais
alpha. Se não houver modelos válidos, experimente outro `.das` ou um
`.drs` de inimigo/jogador; alguns podem ter apenas modelos animados.

Depois escolha um `modelos\\r100_XXX_root.bin` e execute:

```powershell
py r36s-lod\\tools\\r36s_model_lod.py modelos\\r100_XXX_root.bin
py r36s-lod\\tools\\r36s_model_lod.py modelos\\r100_XXX_root.bin --target 0.5 --max-error 0.025 --output meu_modelo_lod.bin
```

**Substitua XXX pelo número exibido**. O primeiro comando é dry-run.
O segundo só grava `meu_modelo_lod.bin` quando há alguma redução
segura no algoritmo. A mensagem `NO_SAFE_REDUCTION_FOUND` é esperada
em muitos modelos; `SKIPPED` significa que não é um modelo estático
elegível. Sem `--output` nunca haverá escrita de arquivo de LOD.

O Windows pode usar `python` em lugar de `py` se o launcher
não existir. O pacote é Python puro, sem requisitos de CUDA, Blender,
Dolphin ou bibliotecas `pip`.

**NÃO copie modelos simplificados para `data/disc0`**: os BIN
originais continuam dentro dos arquivos DAS/DRS, e o port não tem
mecanismo de substituição automática. A versão gerada é para inspeção
e futura integração controlada. Não modifique o único cartão SD
nem os saves.

---

## Como utilizar no PC, somente para análise

No diretório do pacote:

```bash
python3 tools/r36s_model_lod.py meu_modelo.bin
```

Por padrão é **dry run**: imprime um JSON com contagem de triângulos,
candidatos a redução e avisos, mas **não altera arquivos**.

Para gerar um candidato:

```bash
python3 tools/r36s_model_lod.py meu_modelo.bin \
  --target 0.50 --max-error 0.025 \
  --output meu_modelo_lod.bin
```

- `--target 0.50`: meta de manter 50% dos triângulos, **não uma garantia**.
- `--max-error 0.025`: deslocamento/clustering relativo ao diâmetro
  do modelo no espaço local; valor máximo aceito 0.1.
- `--allow-alpha`: experimental **perigoso**, desabilitado por padrão,
  permite simplificar materiais com alpha/cutout — pode eliminar folhas
  e quebrar sombras. Não usar antes de identificar objetos decorativos.

A ferramenta NÃO procura modelos automaticamente em `data/disc0`,
NÃO extrai nem reempacota arquivos DAS, NÃO recompile o núcleo,
e **NÃO instala modelos no jogo**. O usuário precisa fornecer um modelo
`.bin` isolado (extraído de seus arquivos GameCube) para análise.

O R36S não recebe nesse pacote um novo binário de jogo, nem um
arquivo PS2 nativo. O CI valida o conversor com malhas sintéticas;
arquivos proprietários do jogo nunca são enviados ao GitHub.

## Por que ainda não fiz uma versão 100% Low Poly jogável?

A GPU faz centenas de draws por quadro. Diminuir o total de
triângulos e bytes de vértices pode ajudar, mas não diminui por si
o número de draws, mudanças de estado, chamadas `glDrawElements`
e tempo do driver. Muitos modelos com folhagem (texturas alpha)
consistem em **apenas 2 triângulos por folha**: simplificá-los
sem agrupar grupos de folhas ou trocar por uma folha/billboard
LOD talvez não produza redução significativa.

O caminho recomendado para um **port R36S Low Poly jogável** é:

1. **catalogar** modelos `.bin` por sala, material, instância,
   flags de animação, uso em colisão e triângulos visíveis;
2. **escolher somente objetos estáticos/decorativos**, jamais
   inimigos, itens coletáveis, armas, portas, escadas ou elementos
   interativos;
3. criar LODs de 50% e 25% para esses modelos selecionados,
   observando silhueta e texturas em 480×396;
4. integrar a seleção LOD no pipeline de carregamento de recursos
   (ou gerar mod do arquivo de jogo do próprio usuário) com
   identidade verificada, mantendo saves/dados originais;
5. medir FPS e draw calls no mesmo trecho. Se os draws não caírem,
   explorar também **mesh batching** por material e estado de GPU.

Sem os modelos `.bin` extraídos do disco do usuário, ainda não
é possível afirmar quanto pode ganhar ou entregar um mod final.
