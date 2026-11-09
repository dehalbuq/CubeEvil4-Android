# RE4NX-R36S — Fase 3: compatibilidade sintética ARM64/HLE

**Protótipo experimental, não é um emulador Nintendo Switch funcional.**
Não executa Resident Evil 4, NRO/NSO comerciais, nem exige arquivos, firmware,
chaves ou conteúdo original do Switch.

## Resultado real da Fase 2 no R36S

Diagnóstico fornecido pelo usuário (dArkOSRE, kernel 4.4.189 ARM64):
- `PHASE2_FINAL=PASS`, incluindo NSO0/LZ4/SHA256 e limites;
- `ARM64_SYNTHETIC_EXECUTION=PASS RETURN=42 W_X=PASS`;
- `RAM_PROBE=PASS` em 64 MiB; memória disponível no início ~695 MiB;
- `GLES3_PROBE=NOT_REQUESTED` (não há teste GLES nesta compilação).

## Novidades da Fase 3

- `re4nx-hle-aarch64`: segundo executável ARM64 **estático, sem libc**,
  criado com clang + lld e syscalls Linux diretas.
- Simulador **próprio, simplificado** de `svcConnectToNamedPort`,
  `svcSendSyncRequest` e `svcCloseHandle` usando um envelope SFCI/SFCO
  inspirado no CMIF. **Não implementa o protocolo HIPC real**, TLS de
  convidado nem os serviços oficiais do Horizon.
- Tabela de 64 handles com geração e rejeição de handles expirados;
  10.000 aberturas/fechamentos com checagem de vazamento.
- Descoberta sintética de `sm:`, `hid`, `time:u`, `vi:u`, `nvdrv`,
  `fsp-srv` e `appletOE`. **Os serviços não foram implementados:**
  comandos desses handles retornam `UNIMPLEMENTED` intencionalmente.
- Limite contábil de 768 MiB: permite até 704 MiB de heap virtual, com
  reserva teórica de 64 MiB para runtime. **Não é limite real imposto pelo
  kernel** nem demonstra que um jogo de Switch caberia nessa quantidade.
- `mmap` + `mprotect`: 8 MiB escritos e relidos em páginas de 4 KiB;
  direitos RWX são recusados pela política simulada; proteção real R é aplicada.
- Pequena rotina ARM64 sintética executada em memória RX que chama função
  do host pela ABI AAPCS64. **Não intercepta SVCs do Switch**. No PC x86_64,
  esse teste é ignorado; no R36S ele precisa de confirmação real.
- Testes locais de IPC, erros e estresse passaram no PC. O ELF AArch64 foi
  compilado, mas ainda não foi executado no R36S.

## Instalação e testes

1. Extraia todo o ZIP em `/roms/ports/`, preservando a estrutura.
2. No menu Ports, abra `RE4NX-R36S-Prototipo.sh`.
3. Envie o arquivo
   `/roms/ports/re4nx_r36s_prototype/logs/ultimo-re4nx-r36s.log`.

O script executa novamente as Fases 1 e 2 (para detectar regressões),
seguido dos testes Fase 3 e de 10.000 operações com handles. Não roda
nenhum jogo nem modifica seus dados originais.

Resultados desejados **a confirmar em hardware**:

```text
PHASE2_FINAL=PASS
HLE_IPC_SYNTHETIC=PASS
HANDLE_TABLE=PASS MAX=64 OVERFLOW=REJECTED ...
VIRTUAL_MEMORY_BUDGET=PASS ...
ARM64_SYNTHETIC_HOST_CALL=PASS AAPCS64=PASS RX_ONLY=PASS
PHASE3_STRESS_FINAL=PASS
PHASE3_R36S_FINAL=PASS
```

`GLES3_PROBE=NOT_REQUESTED` é esperado. Para solicitar o probe de GPU
com runner C++ **somente quando compilado e incluído separadamente**,
execute `RE4NX_PROBE_GLES3=1 sh RE4NX-R36S-Prototipo.sh`.
O ZIP estático não inclui esse runner ARM64.

## Compilar e validar no Linux

```sh
sh tools/build-native-phase3-aarch64.sh
cc -std=c11 -D_DEFAULT_SOURCE -O2 -Wall -Wextra -Werror \
   src/native_phase3.c -o /tmp/re4nx-hle-host
python3 tests/test_phase3.py /tmp/re4nx-hle-host
```

O projeto inclui a Fase 2 e seus testes. A execução de código comercial
continua desabilitada. A próxima etapa deve estudar serviços Horizon reais
com código de teste autoral, antes de sequer investigar NVN → GLES3.

---

## Arquivo histórico: documentação das Fases 1 e 2

# RE4NX-R36S — Fase 2 nativa ARM64 (protótipo experimental)

**Objetivo:** estudar a possibilidade de uma camada de compatibilidade especializada
no Resident Evil 4 do Nintendo Switch usando Linux AArch64 e até 1 GiB RAM.
**Esta fase ainda NÃO executa o jogo**, firmware Horizon nem shaders NVN.

## Resultado da Fase 1 no hardware real

- R36S dArkOSRE, kernel AArch64 4.4.189: `SYNTHETIC_NATIVE_BOOTSTRAP=PASS`.
- Cabeçalho do arquivo NSO de teste reconhecido: `NSO_INSPECT=HEADER_OK`.
- Memória reportada no início: **895 MiB totais**, **771 MiB disponíveis**, **sem swap**.
- O full runner C++ não estava presente, por isso a Fase 2 adiciona agora um ELF
  ARM64 **estático, sem libc**, compilado com clang + LLD.
- O relógio do console mostrou setembro de 2025; isso não afeta os testes.

## Novidades da Fase 2

- `re4nx-native-aarch64`: **executável Linux AArch64 estático real**, montado com
  syscalls diretas, sem dependência de SDL, glibc ou chaves de Switch.
- Validação dos segmentos `.text`, `.rodata` e `.data` do formato NSO0, com
  verificação de limites e de sobreposição em memória e no arquivo.
- Descompressão **LZ4 raw block** e validação **SHA-256** dos segmentos
  descomprimidos quando exigido pelo cabeçalho.
- Alocação/mapeamento de imagem (máximo de 192 MiB nesta fase), BSS inicializado
  com zero e páginas RX/R/RW no **teste sintético aprovado** (W^X).
- Executa **somente** o código ARM64 público e conhecido de 8 bytes da nossa
  amostra (`mov w0,#42; ret`), após comparar todos os segmentos e validar hashes.
- Prova de RAM separada: reserva, escreve e verifica **64 MiB** em páginas de 4 KiB.
- Registro automático em `re4nx_r36s_prototype/logs/ultimo-re4nx-r36s.log`.

O limite planejado de **768 MiB** para o processo é apenas uma *meta*. Não
é imposto pelo kernel e o teste de 64 MiB não comprova capacidade de rodar
um jogo de Switch em 1 GiB. O loader suporta `--inspect` para um NSO já
legitimamente obtido e descriptografado, **sem executar código desse arquivo**.
Não fornece nem extrai jogos, firmware, chaves, NCA, NSP ou XCI.

## Instalar / testar no R36S

1. Extraia o ZIP em `/roms/ports/`, sobre a pasta da Fase 1 ou em uma pasta nova.
2. Inicie `RE4NX-R36S-Prototipo.sh` pelo menu **Ports**.
3. Envie `re4nx_r36s_prototype/logs/ultimo-re4nx-r36s.log`.

Sem arquivos comerciais, esperamos as linhas abaixo **se o R36S validar tudo**:

```
NSO_INSPECT=PASS LZ4=PASS SHA256=PASS BOUNDS=PASS
ARM64_SYNTHETIC_EXECUTION=PASS RETURN=42 W_X=PASS
RAM_PROBE=PASS
PHASE2_FINAL=PASS
```

As linhas são metas para o teste futuro no aparelho; até agora o teste ARM64
foi **compilado**, e os validadores equivalentes passaram nos testes locais do PC.
Um teste gráfico GLES3 independente permanece como trabalho posterior;
`GLES3_PROBE=NOT_REQUESTED` não indica falta de compatibilidade da GPU.

## Compilar / testar

Em Linux x86_64 com `clang`, `lld`, `gcc` e `python3`:

```
sh tools/build-native-phase2-aarch64.sh
cc -std=c11 -DRE4NX_HOST_TEST -O2 -Wall -Wextra -Werror src/native_phase2.c -o re4nx-native-host
python3 tests/test_phase2.py ./re4nx-native-host
```

Consulte os arquivos da Fase 1 abaixo para o modo de inspeção C++ e o probe
SDL2/GLES3. O código original do GameCube continua em um port independente.

---

# RE4NX-R36S — protótipo experimental para estudar RE4 de Switch

**Fase 1 — inspeção NSO + base ARM64 + diagnóstico GLES 3 / 1 GiB.**

> **Não roda Resident Evil 4, não abre NSP/XCI, não contém firmware, chaves,
> bibliotecas proprietárias, assets do jogo nem execução de código comercial.**
> Este pacote é uma prova de conceito independente do port GameCube.

## Estado verdadeiro

| Etapa | Estado |
|---|---|
| Inspeção do formato NSO0, segmentos `.text`, `.rodata`, `.data`, BSS | Implementado |
| Descompressão de blocos LZ4 com verificações de limites | Implementado |
| Verificação SHA-256 dos segmentos que exigem hash | Implementado |
| Bloqueio opcional por Module ID para fixar uma versão específica do jogo | Implementado no modo inspeção |
| Orçamento de 1 GiB (768 MiB alvo para processo) | Definido/relatado; **não forçado pelo kernel** |
| Teste de execução ARM64 nativa (`mov w0, #42; ret`) | Implementado apenas para amostra sintética no hardware ARM64 |
| Diagnóstico SDL2 + contexto OpenGL ES 3.0+ | Implementado no runner C++ (demanda compilação ARM64) |
| Binário ARM64 estático sem libc, inspeção preliminar NSO | **Compilado**, não validado no R36S ainda |
| Horizon OS / serviços IPC / memória de convidados reais | **Não implementados** |
| NVN/Maxwell → OpenGL ES 3 | **Não implementado** |
| Resident Evil 4 Switch jogável | **Não** |

## O que há no ZIP

- `bootstrap-aarch64` — **ELF AArch64 Linux estático**, sem bibliotecas;
  demonstra lançamento no kernel, registra arquitetura e lê cabeçalho NSO.
- `src/re4nx.cpp` — código-fonte completo C++17 do leitor NSO/LZ4/SHA256,
  autotestes, orçamento de RAM e probe dinâmico SDL2/GLES3.
- `samples/synthetic-aarch64.nso` — amostra aberta, **não é um jogo**.
- `ports/RE4NX-R36S-Prototipo.sh` — script de teste e relatório.
- `CMakeLists.txt`, `tools/`, `.github/workflows/` — build e CI.

## Como testar no R36S imediatamente

1. Extraia **todo o conteúdo do ZIP** diretamente em `/roms/ports/`,
   incluindo `RE4NX-R36S-Prototipo.sh` (na raiz) e a pasta
   `re4nx_r36s_prototype/`.
2. Entre no menu Ports e abra `RE4NX-R36S-Prototipo.sh`, ou rode no terminal:

   ```sh
   sh /roms/ports/re4nx_r36s_prototype/ports/RE4NX-R36S-Prototipo.sh
   ```

3. Envie `re4nx_r36s_prototype/logs/ultimo-re4nx-r36s.log`.

O **bootstrap estático** já deve ser reconhecido pelo kernel ARM64 (sujeito às
configurações do firmware). A execução real do bootstrap **não foi testada**.
O código C++ do runner completo precisa ser recompilado para ARM64 Linux
antes de usar `--selftest` e `--probe-gles3` no console.

Se quiser **somente inspecionar** um `main` NSO **já extraído e descriptografado
legalmente pelo proprietário**, a variável `RE4NX_NSO_PATH` deve apontar para
um arquivo `.nso`. Ela não executa nem modifica nenhum dado do jogo. No runner completo, você
pode opcionalmente exigir o Module ID de uma única versão para impedir que
outro NSO seja aceito: `--inspect main.nso --expect-module-id <32_hex>`.
A identificação de uma build legítima do RE4 ainda não foi fornecida, por
isso nenhuma versão foi cadastrada automaticamente. Não é
necessário fornecer nenhum jogo para o teste sintético.

## Compilar runner C++ no PC Linux

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
build/re4nx --selftest
build/re4nx --inspect samples/synthetic-aarch64.nso
```

Isso compila para a arquitetura **do próprio PC**. O runner ARM64 é compilado
no GitHub Actions por um job Linux AArch64 nativo, ou em um PC com toolchain
AArch64 e bibliotecas de destino. O primeiro binário `bootstrap-aarch64` é
independente de glibc e foi compilado com `clang --target=aarch64-linux-gnu`.

## OpenGL ES 3.0 no R36S

A GPU é verificada com:

```sh
./re4nx --probe-gles3 --fullscreen --seconds 3
```

O teste usa SDL2 via `dlopen` e solicita um contexto ES 3.0; somente limpa a
tela a cada frame. Não emula NVN, não abre shaders do jogo e **não mede o
FPS do RE4**. Necessita SDL2 e driver GLES 3 funcionais no firmware.

## Critério de continuidade

1. Confirmar `SYNTHETIC_NATIVE_BOOTSTRAP=PASS` no kernel do R36S.
2. Compilar runner C++ ARM64 e confirmar `AARCH64_NATIVE_SYNTHETIC=PASS`,
   depois `GLES3_PROBE=PASS` com Mali-G31.
3. Implementar carregamento de imagens e relocações em testes sintéticos.
4. Levantar, com dados legítimos e **sem distribuí-los**, as chamadas HOS e
   NVN necessárias ao RE4; determinar consumo mínimo de RAM antes de qualquer
   promessa de gameplay.

**Referências de formato:**
- https://switchbrew.org/wiki/NSO0
- https://reverseengineering.wiki/index.php/Format:MOD0

**Licença:** MIT, código escrito para este protótipo, sem código de emuladores
de terceiros. Nintendo Switch / Resident Evil são marcas de seus titulares.

## Fase 4: HIPC/CMIF serializado (experimental)

`src/native_phase4.c` implementa um parser limitado dos cabecalhos HIPC
(documentados publicamente) e das assinaturas `SFCI`/`SFCO`, alem de
um **servico sintético** `sm:` e handles com geracoes verificadas.
Ele rejeita descritores de buffer, receive-lists, dominios, mensagens especiais
e versoes que ainda nao implementamos, sem confundi-las com compatibilidade real.

A opcao `--selftest` cobre as mensagens validas, 10 casos negativos e 10 mil
ciclos de handles; `--fuzz` testa 50 mil mensagens pseudoaleatorias com semente
fixa. O ELF Linux ARM64 e estatico e nao executa jogo comercial.

```sh
sh tools/build-native-phase4-aarch64.sh
# Somente em Linux AArch64:
./re4nx-hipc-aarch64 --selftest
./re4nx-hipc-aarch64 --fuzz
```

**Nao implementado:** Horizon real, chamadas SVC de Switch, dominios,
servicos de HID/NVN/arquivo, traducao NVN para GLES3 ou inicializacao do RE4.
Documentacao de referencia: https://switchbrew.org/wiki/HIPC .
