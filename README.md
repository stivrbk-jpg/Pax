# Pax Programming Language 🏛️

**Pax** is a 100% sovereign, native, and minimalist programming language designed for absolute independence from heavy runtimes (no Python, no JVM, no Node.js). It features a strict `[]` bracket syntax and operates entirely on a deterministic Arena-Memory architecture.

---

## 🌟 The 5 Sacred Pillars

Pax is built upon five foundational pillars that guarantee extreme performance, zero memory leaks, and absolute clarity:

1. **Pax `[]` (Arena Memory Architecture):** Complete elimination of the Garbage Collector (GC). Memory is allocated in a ultra-lean, deterministic static block.
2. **Pex `[]` (Native Execution):** Direct compilation into optimized binary bytecode (`.paxbin`) executed by the custom virtual machine.
3. **Iv `[]` (Invariant Verification):** Continuous runtime validation ensuring system and data states remain perfectly sound.
4. **Fw `[]` (Fluid Flow Control):** A clean, elegant syntax relying exclusively on `[]` blocks instead of legacy curly braces `{}` or parentheses `()`.
5. **Fr `[]` (Formal Contracts):** Built-in runtime guardrails (e.g., `need score > 150`) that catch logic errors instantly.

---

## 🚀 Syntax & Core Philosophy

Pax introduces a unified output and display standard. The old `print()` or `say()` commands are completely replaced by the first pillar: **`Pax []`**.

```pax
# Example: Pax v6.0 Pure Bracket Standard
guard on

Pax [
    "=== WELCOME TO PAX ==="
    "The 100% Sovereign Programming Language"
]

score = 100

fn boost [
    score += 50
    Pax [ "Score boosted to:" score ]
]

loop 2 times [
    call boost
]

need score > 150

web on 8080 [
    page "/" [
        Pax [ "<h1>Pax Engine Active</h1><p>Arena Memory & Pax [] Unified.</p>" ]
    ]
]

run
