import React, { useState, useEffect, useCallback, useRef } from 'react';
import { Chessboard } from './components/Chessboard';
import { EvaluationBar } from './components/EvaluationBar';
import { EngineStats } from './components/EngineStats';
import { MoveLogPanel } from './components/MoveLogPanel';
import { GameControls, TimeControlOption } from './components/GameControls';
import { CapturedPieces } from './components/CapturedPieces';
import { RobotHero } from './components/RobotHero';
import { engineService } from './services/EngineService';
import { EngineState, SearchStats } from './types/chess';
import { toggleSound, playMoveSound } from './utils/audio';
import {
  Volume2,
  VolumeX,
  RotateCcw,
  Cpu,
  Zap,
  Shield,
  Layers,
  Sparkles,
  Terminal,
  Activity,
  Award,
  ChevronDown,
  ArrowRight,
  Radio,
  Timer,
  Clock,
} from 'lucide-react';

const LinkedinIcon: React.FC<{ size?: number; color?: string }> = ({ size = 18, color = '#0a66c2' }) => (
  <svg
    width={size}
    height={size}
    viewBox="0 0 24 24"
    fill="currentColor"
    style={{ color, flexShrink: 0 }}
  >
    <path d="M19 3a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h14m-.5 15.5v-5.3a3.26 3.26 0 0 0-3.26-3.26c-.85 0-1.84.52-2.28 1.3v-1.11h-2.79v8.37h2.79v-4.93c0-.77.62-1.4 1.39-1.4a1.4 1.4 0 0 1 1.4 1.4v4.93h2.75M6.88 8.56a1.68 1.68 0 0 0 1.68-1.68c0-.93-.75-1.69-1.68-1.69a1.69 1.69 0 0 0-1.69 1.69c0 .93.76 1.68 1.69 1.68m1.39 9.94v-8.37H5.5v8.37h2.77z" />
  </svg>
);

const INITIAL_FEN = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';

const INITIAL_LEGAL_MOVES = [
  'a2a3', 'a2a4', 'b2b3', 'b2b4', 'c2c3', 'c2c4', 'd2d3', 'd2d4',
  'e2e3', 'e2e4', 'f2f3', 'f2f4', 'g2g3', 'g2g4', 'h2h3', 'h2h4',
  'b1a3', 'b1c3', 'g1f3', 'g1h3',
];

function formatTime(seconds: number | null): string {
  if (seconds === null) return '∞';
  const m = Math.floor(seconds / 60);
  const s = seconds % 60;
  return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
}

export const App: React.FC = () => {
  const [engineState, setEngineState] = useState<EngineState>({
    fen: INITIAL_FEN,
    isWhiteToMove: true,
    inCheck: false,
    isCheckmate: false,
    isStalemate: false,
    legalMoves: INITIAL_LEGAL_MOVES,
    evaluation: 0,
    moveCount: 0,
  });

  const [movesHistory, setMovesHistory] = useState<string[]>([]);
  const [lastMove,     setLastMove]     = useState<string | null>(null);
  const [isThinking,   setIsThinking]   = useState(false);
  const [stats,        setStats]        = useState<SearchStats | null>(null);
  const [isFlipped,    setIsFlipped]    = useState(false);
  const [playerColor,  setPlayerColor]  = useState<'white' | 'black' | 'both' | 'ai'>('white');
  const [searchDepth,  setSearchDepth]  = useState(4);
  const [soundActive,  setSoundActive]  = useState(true);
  const [scrollProgress, setScrollProgress] = useState(0);

  // Time controls: null (unlimited), 5 min, 10 min, 15 min
  const [timeControl, setTimeControl] = useState<TimeControlOption>(10);
  const [whiteTime,   setWhiteTime]   = useState<number | null>(600);
  const [blackTime,   setBlackTime]   = useState<number | null>(600);
  const [isTimeOut,   setIsTimeOut]   = useState<'white' | 'black' | null>(null);

  const stateRef    = useRef(engineState);
  stateRef.current  = engineState;
  const thinkingRef = useRef(isThinking);
  thinkingRef.current = isThinking;

  // Track dynamic scroll progress
  useEffect(() => {
    const handleScroll = () => {
      const totalScroll = document.documentElement.scrollHeight - window.innerHeight;
      if (totalScroll > 0) {
        setScrollProgress((window.scrollY / totalScroll) * 100);
      }
    };
    window.addEventListener('scroll', handleScroll, { passive: true });
    return () => window.removeEventListener('scroll', handleScroll);
  }, []);

  useEffect(() => {
    engineService.onStateChange((s) => {
      setEngineState(s);
    });

    engineService.getLegalMoves().then((moves) => {
      setEngineState((prev) => ({
        ...prev,
        legalMoves: moves && moves.length > 0 ? moves : prev.legalMoves,
      }));
    });

    return () => {
      engineService.terminate();
    };
  }, []);

  // Handle Time Control changes & Resets
  const handleTimeControlChange = (option: TimeControlOption) => {
    setTimeControl(option);
    const initialSeconds = option ? option * 60 : null;
    setWhiteTime(initialSeconds);
    setBlackTime(initialSeconds);
    setIsTimeOut(null);
  };

  // Clock countdown interval
  useEffect(() => {
    if (timeControl === null || movesHistory.length === 0) return;
    if (engineState.isCheckmate || engineState.isStalemate || isTimeOut) return;

    const timer = setInterval(() => {
      if (engineState.isWhiteToMove) {
        setWhiteTime((prev) => {
          if (prev === null) return null;
          if (prev <= 1) {
            setIsTimeOut('white');
            return 0;
          }
          return prev - 1;
        });
      } else {
        setBlackTime((prev) => {
          if (prev === null) return null;
          if (prev <= 1) {
            setIsTimeOut('black');
            return 0;
          }
          return prev - 1;
        });
      }
    }, 1000);

    return () => clearInterval(timer);
  }, [timeControl, movesHistory.length, engineState.isWhiteToMove, engineState.isCheckmate, engineState.isStalemate, isTimeOut]);

  // AI Move Execution with sound feedback
  const triggerAiMove = useCallback(async () => {
    if (thinkingRef.current) return;
    const cur = stateRef.current;
    if (cur.isCheckmate || cur.isStalemate || isTimeOut) return;

    setIsThinking(true);
    try {
      const res = await engineService.getBestMove(2500, searchDepth);
      if (res.bestMove) {
        setStats(res.stats);
        setLastMove(res.bestMove);
        setMovesHistory((prev) => [...prev, res.bestMove]);

        // Play audio for AI move
        playMoveSound(false, cur.inCheck);

        await engineService.makeMove(res.bestMove);
      }
    } catch (err) {
      console.error('[AI] Search failed:', err);
    } finally {
      setIsThinking(false);
    }
  }, [searchDepth, isTimeOut]);

  // Turn management
  useEffect(() => {
    if (isThinking || isTimeOut) return;
    if (engineState.isCheckmate || engineState.isStalemate) return;

    const isAiTurn =
      (playerColor === 'white' && !engineState.isWhiteToMove) ||
      (playerColor === 'black' &&  engineState.isWhiteToMove) ||
      playerColor === 'ai';

    if (isAiTurn) {
      const delay = playerColor === 'ai' ? 450 : 250;
      const t = setTimeout(() => triggerAiMove(), delay);
      return () => clearTimeout(t);
    }
  }, [
    engineState.isWhiteToMove,
    playerColor,
    engineState.isCheckmate,
    engineState.isStalemate,
    isThinking,
    isTimeOut,
    triggerAiMove,
  ]);

  // Human Move Execution
  const handleMakeMove = useCallback(async (uci: string) => {
    if (isThinking || isTimeOut) return;

    setLastMove(uci);
    setMovesHistory((prev) => [...prev, uci]);

    const res = await engineService.makeMove(uci);
    if (!res.success) {
      setMovesHistory((prev) => prev.slice(0, -1));
    }
  }, [isThinking, isTimeOut]);

  const handleNewGame = useCallback(async () => {
    engineService.stopSearch();
    setIsThinking(false);
    setLastMove(null);
    setMovesHistory([]);
    setStats(null);
    setIsTimeOut(null);
    const initialSeconds = timeControl ? timeControl * 60 : null;
    setWhiteTime(initialSeconds);
    setBlackTime(initialSeconds);
    const newState = await engineService.newGame();
    setEngineState(newState);
  }, [timeControl]);

  const handleUndo = useCallback(async () => {
    if (movesHistory.length === 0 || isThinking || isTimeOut) return;
    engineService.stopSearch();
    setIsThinking(false);

    if (playerColor === 'white' || playerColor === 'black') {
      await engineService.undoMove();
      if (movesHistory.length >= 2) {
        await engineService.undoMove();
        setMovesHistory((prev) => prev.slice(0, -2));
      } else {
        setMovesHistory((prev) => prev.slice(0, -1));
      }
    } else {
      await engineService.undoMove();
      setMovesHistory((prev) => prev.slice(0, -1));
    }
    setLastMove(null);
  }, [movesHistory.length, isThinking, isTimeOut, playerColor]);

  const handleSoundToggle = () => {
    const updated = toggleSound();
    setSoundActive(updated);
  };

  const scrollToArena = () => {
    const arena = document.getElementById('arena-section');
    arena?.scrollIntoView({ behavior: 'smooth' });
  };

  const scrollToCapabilities = () => {
    const cap = document.getElementById('capabilities-section');
    cap?.scrollIntoView({ behavior: 'smooth' });
  };

  const isWhiteTurn = engineState.isWhiteToMove;
  const isOpponentTurn = playerColor === 'black' ? isWhiteTurn : !isWhiteTurn;
  const isUserTurn = !isOpponentTurn;

  const topPlayerColor = playerColor === 'black' ? 'white' : 'black';
  const bottomPlayerColor = playerColor === 'black' ? 'black' : 'white';

  const topPlayerTime = topPlayerColor === 'white' ? whiteTime : blackTime;
  const bottomPlayerTime = bottomPlayerColor === 'white' ? whiteTime : blackTime;

  const isTopPlayerLow = topPlayerTime !== null && topPlayerTime <= 30;
  const isBottomPlayerLow = bottomPlayerTime !== null && bottomPlayerTime <= 30;

  return (
    <div style={{ minHeight: '100vh', background: 'var(--bg-studio-dark)', color: '#fff', overflowX: 'hidden' }}>
      {/* Dynamic Scroll Progress Bar */}
      <div className="scroll-progress-bar" style={{ width: `${scrollProgress}%` }} />

      {/* ====================================================================
          1. DYNAMIC TOP NAVIGATION BAR
          ==================================================================== */}
      <header className="app-header">
        <div
          style={{ display: 'flex', alignItems: 'center', gap: 10, cursor: 'pointer' }}
          onClick={() => window.scrollTo({ top: 0, behavior: 'smooth' })}
        >
          <span
            style={{
              fontFamily: 'var(--font-display)',
              fontWeight: 800,
              fontSize: 'clamp(16px, 1.8vw, 19px)',
              letterSpacing: '-0.03em',
              color: '#0e0f13',
              mixBlendMode: 'difference',
            }}
          >
            AlphaOne®
          </span>
          <span
            style={{
              fontSize: 10.5,
              fontFamily: 'var(--font-mono)',
              color: '#888',
              letterSpacing: '0.04em',
              textTransform: 'uppercase',
              border: '1px solid rgba(255,255,255,0.12)',
              padding: '2px 6px',
              borderRadius: 4,
            }}
          >
            C++17 / WASM
          </span>
        </div>

        {/* Center Minimal Geometric Brand Mark */}
        <div
          style={{
            width: 26,
            height: 26,
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            color: '#0e0f13',
            mixBlendMode: 'difference',
            cursor: 'pointer',
          }}
          onClick={() => window.scrollTo({ top: 0, behavior: 'smooth' })}
          title="AlphaOne Home"
        >
          <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
            <circle cx="12" cy="12" r="9" />
            <path d="M12 3a9 9 0 0 0 0 18v-18z" fill="currentColor" />
          </svg>
        </div>

        {/* Right Navigation Actions */}
        <div style={{ display: 'flex', alignItems: 'center', gap: 'clamp(8px, 1.5vw, 16px)' }}>
          <button
            onClick={scrollToCapabilities}
            style={{
              background: 'none',
              border: 'none',
              color: '#111',
              fontFamily: 'var(--font-sans)',
              fontSize: 13,
              fontWeight: 600,
              letterSpacing: '-0.01em',
              cursor: 'pointer',
              mixBlendMode: 'difference',
              padding: '6px 8px',
            }}
          >
            Capabilities
          </button>
          <button onClick={scrollToArena} className="loco-pill">
            <span>Play Arena</span>
            <ArrowRight size={13} />
          </button>
        </div>
      </header>

      {/* ====================================================================
          2. HERO SECTION (Dynamic 3D Character Studio)
          ==================================================================== */}
      <section id="hero-section" className="hero-container">
        {/* Studio Lighting Vignette */}
        <div
          style={{
            position: 'absolute',
            inset: 0,
            background: 'radial-gradient(circle at 50% 90%, rgba(0,0,0,0.38) 0%, transparent 70%)',
            pointerEvents: 'none',
          }}
        />

        {/* Left Column: Typography & Interactive Badges */}
        <div className="hero-left-col">
          <div>
            <div
              style={{
                display: 'inline-flex',
                alignItems: 'center',
                gap: 6,
                padding: '4px 10px',
                borderRadius: 99,
                backgroundColor: 'rgba(14, 15, 19, 0.08)',
                marginBottom: 10,
              }}
            >
              <Radio size={12} color="#0e0f13" />
              <span style={{ fontFamily: 'var(--font-mono)', fontSize: 11, fontWeight: 700, color: '#0e0f13' }}>
                LIVE ENGINE STUDIO
              </span>
            </div>
            <p
              style={{
                fontFamily: 'var(--font-sans)',
                fontSize: 'clamp(13px, 1.2vw, 15px)',
                color: '#464852',
                lineHeight: 1.5,
                fontWeight: 500,
              }}
            >
              Hi there, I am AlphaOne — low latency chess intelligence.
            </p>
          </div>

          <h1
            style={{
              fontFamily: 'var(--font-display)',
              fontSize: 'clamp(26px, 3.8vw, 48px)',
              fontWeight: 700,
              lineHeight: 1.15,
              letterSpacing: '-0.035em',
              color: '#0e0f13',
            }}
          >
            AlphaOne — High-Performance Chess Engine developed in C++ with ELO ~1900.
            <span className="blinking-cursor">▌</span>
          </h1>

          <div className="hero-pills-row" style={{ display: 'flex', flexWrap: 'wrap', gap: 8, marginTop: 4 }}>
            <button onClick={scrollToArena} className="loco-pill">
              <Sparkles size={13} />
              Launch Arena
            </button>
            <button onClick={scrollToArena} className="loco-pill">
              <Timer size={13} />
              {timeControl ? `${timeControl} Min Timer` : 'Timed Clock'}
            </button>
            <button onClick={scrollToCapabilities} className="loco-pill">
              <Zap size={13} />
              4,000,000+ NPS
            </button>
            <button onClick={scrollToCapabilities} className="loco-pill">
              <Cpu size={13} />
              WebAssembly Core
            </button>
            <button onClick={scrollToCapabilities} className="loco-pill">
              <Activity size={13} />
              Sub-5ms Latency
            </button>
          </div>
        </div>

        {/* 3D Robot Head Assembly */}
        <div className="hero-robot-wrapper">
          <RobotHero />
        </div>

        {/* Dynamic Sound & Reset Floats */}
        <div
          style={{
            position: 'absolute',
            bottom: 20,
            left: 'var(--gutter-x)',
            zIndex: 20,
            display: 'flex',
            alignItems: 'center',
            gap: 10,
          }}
        >
          <button
            onClick={() => window.location.reload()}
            title="Reload Experience"
            style={{
              width: 36,
              height: 36,
              borderRadius: '50%',
              backgroundColor: '#0e0f13',
              color: '#fff',
              border: 'none',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              cursor: 'pointer',
              boxShadow: '0 4px 14px rgba(0,0,0,0.25)',
              transition: 'transform 0.2s ease',
            }}
            onMouseEnter={(e) => (e.currentTarget.style.transform = 'rotate(-90deg) scale(1.08)')}
            onMouseLeave={(e) => (e.currentTarget.style.transform = 'rotate(0deg) scale(1)')}
          >
            <RotateCcw size={14} />
          </button>

          <button
            onClick={handleSoundToggle}
            title={soundActive ? 'Mute Audio' : 'Enable Audio'}
            style={{
              display: 'inline-flex',
              alignItems: 'center',
              gap: 6,
              padding: '7px 14px',
              borderRadius: 9999,
              backgroundColor: '#0e0f13',
              color: '#fff',
              border: 'none',
              fontSize: 11.5,
              fontWeight: 600,
              fontFamily: 'var(--font-sans)',
              cursor: 'pointer',
              boxShadow: '0 4px 14px rgba(0,0,0,0.25)',
              transition: 'all 0.2s ease',
            }}
          >
            {soundActive ? <Volume2 size={13} /> : <VolumeX size={13} />}
            <span>{soundActive ? 'Audio ON' : 'MUTE'}</span>
          </button>
        </div>

        {/* Scroll Indicator */}
        <div
          onClick={scrollToCapabilities}
          style={{
            position: 'absolute',
            bottom: 18,
            right: 'var(--gutter-x)',
            zIndex: 20,
            display: 'flex',
            alignItems: 'center',
            gap: 4,
            cursor: 'pointer',
            opacity: 0.8,
            transition: 'opacity 0.2s ease',
          }}
        >
          <span style={{ fontSize: 10.5, fontFamily: 'var(--font-mono)', color: '#25262c', fontWeight: 700 }}>
            EXPLORE
          </span>
          <ChevronDown size={14} color="#25262c" className="pulse-soft" />
        </div>
      </section>

      {/* ====================================================================
          3. CAPABILITIES & ARCHITECTURE SECTION
          ==================================================================== */}
      <section
        id="capabilities-section"
        style={{
          padding: 'clamp(60px, 8vw, 110px) var(--gutter-x)',
          maxWidth: 'var(--container-max-w)',
          margin: '0 auto',
          display: 'flex',
          flexDirection: 'column',
          gap: 'clamp(36px, 5vw, 60px)',
        }}
      >
        <div style={{ display: 'flex', flexDirection: 'column', gap: 12 }}>
          <span
            style={{
              fontSize: 11,
              fontFamily: 'var(--font-mono)',
              color: 'var(--accent-cyan)',
              letterSpacing: '0.12em',
              fontWeight: 700,
            }}
          >
            01 / ARCHITECTURE & METRICS
          </span>
          <h2
            style={{
              fontFamily: 'var(--font-display)',
              fontSize: 'clamp(24px, 3.2vw, 42px)',
              fontWeight: 700,
              letterSpacing: '-0.03em',
              color: '#f4f4f7',
              maxWidth: 750,
            }}
          >
            High-Performance C++17 Core Compiled to WebAssembly
          </h2>
          <p style={{ fontSize: 'clamp(13.5px, 1.2vw, 15px)', color: '#888b96', maxWidth: 640, lineHeight: 1.6 }}>
            AlphaOne provides pure low-latency client-side computation with bitboard move generation,
            iterative deepening alpha-beta pruning, and 64-bit Zobrist transposition memoization.
          </p>
        </div>

        {/* Bento Grid */}
        <div className="capabilities-grid">
          <div className="studio-card">
            <div style={{ display: 'flex', alignItems: 'center', gap: 12, marginBottom: 14 }}>
              <div
                style={{
                  width: 36,
                  height: 36,
                  borderRadius: 10,
                  backgroundColor: 'rgba(0, 242, 254, 0.1)',
                  border: '1px solid rgba(0, 242, 254, 0.25)',
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                }}
              >
                <Zap size={18} color="var(--accent-cyan)" />
              </div>
              <span style={{ fontSize: 11, fontFamily: 'var(--font-mono)', color: '#8b8e99' }}>THROUGHPUT</span>
            </div>
            <h3 style={{ fontSize: 'clamp(20px, 1.8vw, 24px)', fontWeight: 700, marginBottom: 8, letterSpacing: '-0.02em' }}>
              1,000,000+ NPS
            </h3>
            <p style={{ fontSize: 13, color: '#9da0aa', lineHeight: 1.6 }}>
              Inner search loops run with zero dynamic heap allocations. Bitboard state caching and raycasting enable
              sub-millisecond evaluation across millions of board transitions.
            </p>
          </div>

          <div className="studio-card">
            <div style={{ display: 'flex', alignItems: 'center', gap: 12, marginBottom: 14 }}>
              <div
                style={{
                  width: 36,
                  height: 36,
                  borderRadius: 10,
                  backgroundColor: 'rgba(16, 185, 129, 0.1)',
                  border: '1px solid rgba(16, 185, 129, 0.25)',
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                }}
              >
                <Activity size={18} color="var(--accent-emerald)" />
              </div>
              <span style={{ fontSize: 11, fontFamily: 'var(--font-mono)', color: '#8b8e99' }}>LATENCY</span>
            </div>
            <h3 style={{ fontSize: 'clamp(20px, 1.8vw, 24px)', fontWeight: 700, marginBottom: 8, letterSpacing: '-0.02em' }}>
              Sub-5ms Tactical Replies
            </h3>
            <p style={{ fontSize: 13, color: '#9da0aa', lineHeight: 1.6 }}>
              Moves are calculated locally in your browser's dedicated Web Worker thread. Zero server hops or API roundtrips
              provide instantaneous tactical response.
            </p>
          </div>

          <div className="studio-card">
            <div style={{ display: 'flex', alignItems: 'center', gap: 12, marginBottom: 14 }}>
              <div
                style={{
                  width: 36,
                  height: 36,
                  borderRadius: 10,
                  backgroundColor: 'rgba(245, 158, 11, 0.1)',
                  border: '1px solid rgba(245, 158, 11, 0.25)',
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                }}
              >
                <Award size={18} color="var(--accent-amber)" />
              </div>
              <span style={{ fontSize: 11, fontFamily: 'var(--font-mono)', color: '#8b8e99' }}>RATING</span>
            </div>
            <h3 style={{ fontSize: 'clamp(20px, 1.8vw, 24px)', fontWeight: 700, marginBottom: 8, letterSpacing: '-0.02em' }}>
              ELO ~1900 Bitboard & Tapered Search
            </h3>
            <p style={{ fontSize: 13, color: '#9da0aa', lineHeight: 1.6 }}>
              Utilizes 64-bit bitboards, PeSTO tapered piece-square tables, MVV-LVA move ordering, Quiescence search,
              and iterative deepening Negamax search up to depth 7.
            </p>
          </div>

          <div className="studio-card">
            <div style={{ display: 'flex', alignItems: 'center', gap: 12, marginBottom: 14 }}>
              <div
                style={{
                  width: 36,
                  height: 36,
                  borderRadius: 10,
                  backgroundColor: 'rgba(59, 130, 246, 0.1)',
                  border: '1px solid rgba(59, 130, 246, 0.25)',
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                }}
              >
                <Layers size={18} color="var(--accent-blue)" />
              </div>
              <span style={{ fontSize: 11, fontFamily: 'var(--font-mono)', color: '#8b8e99' }}>CACHE</span>
            </div>
            <h3 style={{ fontSize: 'clamp(20px, 1.8vw, 24px)', fontWeight: 700, marginBottom: 8, letterSpacing: '-0.02em' }}>
              64-bit Zobrist Memoization
            </h3>
            <p style={{ fontSize: 13, color: '#9da0aa', lineHeight: 1.6 }}>
              Depth-aware transposition table with exact, lower-bound, and upper-bound pruning cutoffs prevents redundant
              branch recalculation in deep tactical lines.
            </p>
          </div>

          <div className="studio-card">
            <div style={{ display: 'flex', alignItems: 'center', gap: 12, marginBottom: 14 }}>
              <div
                style={{
                  width: 36,
                  height: 36,
                  borderRadius: 10,
                  backgroundColor: 'rgba(255, 255, 255, 0.08)',
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                }}
              >
                <Shield size={18} color="#fff" />
              </div>
              <span style={{ fontSize: 11, fontFamily: 'var(--font-mono)', color: '#8b8e99' }}>CLIENT-SIDE</span>
            </div>
            <h3 style={{ fontSize: 'clamp(20px, 1.8vw, 24px)', fontWeight: 700, marginBottom: 8, letterSpacing: '-0.02em' }}>
              100% Offline WebAssembly
            </h3>
            <p style={{ fontSize: 13, color: '#9da0aa', lineHeight: 1.6 }}>
              Compiled with Emscripten using -O3 optimization flags. No backend servers, no analytics beacons, and no telemetry
              overhead. Runs directly in any modern browser.
            </p>
          </div>

          <div className="studio-card">
            <div style={{ display: 'flex', alignItems: 'center', gap: 12, marginBottom: 14 }}>
              <div
                style={{
                  width: 36,
                  height: 36,
                  borderRadius: 10,
                  backgroundColor: 'rgba(255, 255, 255, 0.08)',
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                }}
              >
                <Terminal size={18} color="#fff" />
              </div>
              <span style={{ fontSize: 11, fontFamily: 'var(--font-mono)', color: '#8b8e99' }}>CROSS-PLATFORM</span>
            </div>
            <h3 style={{ fontSize: 'clamp(20px, 1.8vw, 24px)', fontWeight: 700, marginBottom: 8, letterSpacing: '-0.02em' }}>
              Standalone Native CLI
            </h3>
            <p style={{ fontSize: 13, color: '#9da0aa', lineHeight: 1.6 }}>
              The C++ engine also compiles to a native standalone executable (`alphaone_cli`) for terminal-based benchmarking,
              UCI interaction, and headless testing.
            </p>
          </div>
        </div>
      </section>

      {/* ====================================================================
          4. THE ACTUAL PRODUCT / THE CHESS ARENA SECTION
          ==================================================================== */}
      <section
        id="arena-section"
        style={{
          padding: 'clamp(40px, 6vw, 80px) var(--gutter-x) clamp(60px, 10vw, 120px)',
          maxWidth: 'var(--container-max-w)',
          margin: '0 auto',
          display: 'flex',
          flexDirection: 'column',
          gap: 'clamp(24px, 3.5vw, 36px)',
        }}
      >
        {/* Section Header */}
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-end', flexWrap: 'wrap', gap: 16 }}>
          <div>
            <span
              style={{
                fontSize: 11,
                fontFamily: 'var(--font-mono)',
                color: 'var(--accent-cyan)',
                letterSpacing: '0.12em',
                fontWeight: 700,
              }}
            >
              02 / THE ARENA
            </span>
            <h2
              style={{
                fontFamily: 'var(--font-display)',
                fontSize: 'clamp(24px, 3.2vw, 40px)',
                fontWeight: 700,
                letterSpacing: '-0.03em',
                color: '#fff',
                marginTop: 4,
              }}
            >
              Play Against AlphaOne
            </h2>
          </div>

          <div style={{ display: 'flex', gap: 8, flexWrap: 'wrap' }}>
            <button onClick={handleNewGame} className="loco-pill-dark">
              <RotateCcw size={13} />
              <span>Reset Game</span>
            </button>
            <button onClick={() => setIsFlipped((prev) => !prev)} className="loco-pill-dark">
              <span>Flip Board</span>
            </button>
          </div>
        </div>

        {/* Main Chess Arena Responsive Grid */}
        <div className="arena-grid">
          {/* Left Column: Board Container Box */}
          <div
            className="card"
            style={{
              padding: 'clamp(12px, 2vw, 20px)',
              display: 'flex',
              flexDirection: 'column',
              gap: 'clamp(10px, 1.5vw, 14px)',
              backgroundColor: 'var(--bg-card)',
            }}
          >
            {/* Top Opponent Player Badge */}
            <div
              className={`player-badge ${isOpponentTurn ? 'player-active-card' : ''}`}
              style={{
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'space-between',
                padding: 'clamp(8px, 1.2vw, 12px) clamp(10px, 1.5vw, 16px)',
                borderRadius: 10,
                background: 'rgba(255,255,255,0.03)',
                border: '1px solid var(--border-subtle)',
                transition: 'all 0.25s ease',
              }}
            >
              <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
                <div
                  style={{
                    width: 32,
                    height: 32,
                    borderRadius: 8,
                    background: '#1d1f26',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    fontSize: 15,
                    border: '1px solid rgba(255,255,255,0.1)',
                  }}
                >
                  {playerColor === 'black' ? '👤' : '🤖'}
                </div>
                <div>
                  <div style={{ fontSize: 'clamp(12px, 1.1vw, 13.5px)', fontWeight: 700 }}>
                    {playerColor === 'black' ? 'You (Black)' : 'AlphaOne AI'}
                  </div>
                  <div style={{ fontSize: 10.5, color: '#888' }}>
                    {playerColor === 'black' ? 'Human Player' : `Depth ${searchDepth} (ELO ~1900)`}
                  </div>
                </div>
              </div>

              <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
                <CapturedPieces fen={engineState.fen} />

                {/* Digital Clock for Top Player */}
                {timeControl !== null && (
                  <div
                    style={{
                      display: 'flex',
                      alignItems: 'center',
                      gap: 5,
                      padding: '4px 10px',
                      borderRadius: 6,
                      backgroundColor: isTopPlayerLow
                        ? 'rgba(244, 63, 94, 0.15)'
                        : isOpponentTurn
                        ? 'rgba(0, 242, 254, 0.12)'
                        : 'rgba(255, 255, 255, 0.05)',
                      border: isTopPlayerLow
                        ? '1px solid rgba(244, 63, 94, 0.4)'
                        : isOpponentTurn
                        ? '1px solid rgba(0, 242, 254, 0.3)'
                        : '1px solid var(--border-subtle)',
                      fontFamily: 'var(--font-mono)',
                      fontSize: 'clamp(12px, 1.1vw, 14px)',
                      fontWeight: 700,
                      color: isTopPlayerLow ? 'var(--accent-rose)' : isOpponentTurn ? '#ffffff' : 'var(--text-muted)',
                      boxShadow: isOpponentTurn ? '0 0 10px rgba(0,242,254,0.15)' : 'none',
                      transition: 'all 0.2s ease',
                    }}
                  >
                    <Clock size={12} />
                    <span>{formatTime(topPlayerTime)}</span>
                  </div>
                )}
              </div>
            </div>

            {/* Board + Integrated Dynamic Evaluation Bar */}
            <div className="board-eval-container">
              <EvaluationBar score={engineState.evaluation} isWhiteBottom={!isFlipped} />
              <Chessboard
                fen={engineState.fen}
                isFlipped={isFlipped}
                legalMoves={engineState.legalMoves}
                lastMove={lastMove}
                inCheck={engineState.inCheck}
                isWhiteToMove={engineState.isWhiteToMove}
                onMakeMove={handleMakeMove}
                disabled={
                  isThinking ||
                  Boolean(isTimeOut) ||
                  (playerColor === 'white' && !engineState.isWhiteToMove) ||
                  (playerColor === 'black' &&  engineState.isWhiteToMove) ||
                  playerColor === 'ai'
                }
              />
            </div>

            {/* Bottom Player Badge */}
            <div
              className={`player-badge ${isUserTurn ? 'player-active-card' : ''}`}
              style={{
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'space-between',
                padding: 'clamp(8px, 1.2vw, 12px) clamp(10px, 1.5vw, 16px)',
                borderRadius: 10,
                background: 'rgba(255,255,255,0.03)',
                border: '1px solid var(--border-subtle)',
                transition: 'all 0.25s ease',
              }}
            >
              <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
                <div
                  style={{
                    width: 32,
                    height: 32,
                    borderRadius: 8,
                    background: '#f2f3f5',
                    color: '#111',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    fontSize: 15,
                  }}
                >
                  {playerColor === 'black' ? '🤖' : '👤'}
                </div>
                <div>
                  <div style={{ fontSize: 'clamp(12px, 1.1vw, 13.5px)', fontWeight: 700 }}>
                    {playerColor === 'black' ? 'AlphaOne AI (White)' : 'You (White)'}
                  </div>
                  <div style={{ fontSize: 10.5, color: isUserTurn ? 'var(--accent-emerald)' : '#888', fontWeight: isUserTurn ? 700 : 400 }}>
                    {playerColor === 'black' ? `Depth ${searchDepth} (ELO ~1900)` : (isUserTurn ? 'Active Turn' : 'Waiting')}
                  </div>
                </div>
              </div>

              <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
                <div style={{ fontSize: 11, fontFamily: 'var(--font-mono)', color: '#888' }}>
                  Move #{movesHistory.length}
                </div>

                {/* Digital Clock for Bottom Player */}
                {timeControl !== null && (
                  <div
                    style={{
                      display: 'flex',
                      alignItems: 'center',
                      gap: 5,
                      padding: '4px 10px',
                      borderRadius: 6,
                      backgroundColor: isBottomPlayerLow
                        ? 'rgba(244, 63, 94, 0.15)'
                        : isUserTurn
                        ? 'rgba(16, 185, 129, 0.15)'
                        : 'rgba(255, 255, 255, 0.05)',
                      border: isBottomPlayerLow
                        ? '1px solid rgba(244, 63, 94, 0.4)'
                        : isUserTurn
                        ? '1px solid rgba(16, 185, 129, 0.35)'
                        : '1px solid var(--border-subtle)',
                      fontFamily: 'var(--font-mono)',
                      fontSize: 'clamp(12px, 1.1vw, 14px)',
                      fontWeight: 700,
                      color: isBottomPlayerLow ? 'var(--accent-rose)' : isUserTurn ? '#ffffff' : 'var(--text-muted)',
                      boxShadow: isUserTurn ? '0 0 10px rgba(16,185,129,0.15)' : 'none',
                      transition: 'all 0.2s ease',
                    }}
                  >
                    <Clock size={12} />
                    <span>{formatTime(bottomPlayerTime)}</span>
                  </div>
                )}
              </div>
            </div>

            {/* Dynamic Game Alerts */}
            {isTimeOut && (
              <div
                style={{
                  padding: '12px 18px',
                  borderRadius: 10,
                  backgroundColor: 'rgba(239, 68, 68, 0.15)',
                  border: '1px solid rgba(239, 68, 68, 0.4)',
                  color: '#fff',
                  fontWeight: 700,
                  fontSize: 13,
                  textAlign: 'center',
                  boxShadow: '0 0 16px rgba(239, 68, 68, 0.3)',
                }}
              >
                ⏱️ Time Out! {isTimeOut === 'white' ? 'White' : 'Black'} ran out of time. {isTimeOut === 'white' ? 'Black' : 'White'} wins!
              </div>
            )}
            {engineState.isCheckmate && (
              <div
                style={{
                  padding: '12px 18px',
                  borderRadius: 10,
                  backgroundColor: 'rgba(239, 68, 68, 0.15)',
                  border: '1px solid rgba(239, 68, 68, 0.4)',
                  color: '#fff',
                  fontWeight: 700,
                  fontSize: 13,
                  textAlign: 'center',
                  boxShadow: '0 0 16px rgba(239, 68, 68, 0.3)',
                }}
              >
                Checkmate! {engineState.isWhiteToMove ? 'Black' : 'White'} wins the game!
              </div>
            )}
            {engineState.isStalemate && (
              <div
                style={{
                  padding: '12px 18px',
                  borderRadius: 10,
                  backgroundColor: 'rgba(245, 158, 11, 0.15)',
                  border: '1px solid rgba(245, 158, 11, 0.4)',
                  color: '#fff',
                  fontWeight: 700,
                  fontSize: 13,
                  textAlign: 'center',
                  boxShadow: '0 0 16px rgba(245, 158, 11, 0.3)',
                }}
              >
                Stalemate! Draw by lack of legal moves.
              </div>
            )}
            {engineState.inCheck && !engineState.isCheckmate && !isTimeOut && (
              <div
                style={{
                  padding: '8px 14px',
                  borderRadius: 8,
                  backgroundColor: 'rgba(244, 63, 94, 0.12)',
                  border: '1px solid rgba(244, 63, 94, 0.3)',
                  color: 'var(--accent-rose)',
                  fontWeight: 700,
                  fontSize: 12,
                  textAlign: 'center',
                }}
              >
                ⚠️ Check! {engineState.isWhiteToMove ? 'White' : 'Black'} King is under attack.
              </div>
            )}
          </div>

          {/* Right Column: Engine Console Boxes */}
          <div style={{ display: 'flex', flexDirection: 'column', gap: 'clamp(14px, 2vw, 20px)' }}>
            <EngineStats stats={stats} isThinking={isThinking} />

            <GameControls
              onNewGame={handleNewGame}
              onUndo={handleUndo}
              onFlipBoard={() => setIsFlipped((prev) => !prev)}
              isThinking={isThinking}
              onStopSearch={() => engineService.stopSearch()}
              depth={searchDepth}
              onDepthChange={setSearchDepth}
              playerColor={playerColor}
              onPlayerColorChange={setPlayerColor}
              timeControl={timeControl}
              onTimeControlChange={handleTimeControlChange}
              moveCount={movesHistory.length}
            />

            <MoveLogPanel moves={movesHistory} />
          </div>
        </div>
      </section>

      {/* ====================================================================
          5. DYNAMIC FOOTER
          ==================================================================== */}
      <footer
        style={{
          borderTop: '1px solid rgba(255,255,255,0.08)',
          padding: 'clamp(36px, 5vw, 48px) var(--gutter-x) clamp(28px, 4vw, 40px)',
          display: 'flex',
          flexDirection: 'column',
          gap: 28,
          color: '#777a84',
          fontSize: 13,
          backgroundColor: '#0a0b0e',
        }}
      >
        {/* Meet the Developers Section */}
        <div
          style={{
            display: 'flex',
            flexDirection: 'column',
            gap: 14,
            paddingBottom: 24,
            borderBottom: '1px solid rgba(255,255,255,0.06)',
          }}
        >
          <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
            <span
              style={{
                fontFamily: 'var(--font-mono)',
                fontSize: 11,
                fontWeight: 700,
                textTransform: 'uppercase',
                letterSpacing: '0.12em',
                color: 'var(--accent-silver)',
              }}
            >
              Meet the Developers
            </span>
          </div>

          <div
            style={{
              display: 'flex',
              alignItems: 'center',
              flexWrap: 'wrap',
              gap: 12,
            }}
          >
            <a
              href="https://www.linkedin.com/in/naman-ostwal-918894230/"
              target="_blank"
              rel="noopener noreferrer"
              className="developer-link"
            >
              <LinkedinIcon size={17} color="#0a66c2" />
              <span>Naman Ostwal</span>
            </a>

            <a
              href="https://www.linkedin.com/in/puneet-rathi-513465286"
              target="_blank"
              rel="noopener noreferrer"
              className="developer-link"
            >
              <LinkedinIcon size={17} color="#0a66c2" />
              <span>Puneet Rathi</span>
            </a>
          </div>
        </div>

        {/* Bottom meta info */}
        <div className="footer-inner">
          <div>
            <span style={{ fontWeight: 700, color: '#d1d4dc' }}>AlphaOne Chess Engine</span>
            {' — '}
            Modern C++17 ported to WebAssembly
          </div>
          <div style={{ display: 'flex', gap: 'clamp(10px, 2vw, 20px)', flexWrap: 'wrap', fontSize: 12 }}>
            <span>100% Client-Side Compute</span>
            <span>Zero Server Latency</span>
            <span>Zero Pygame Dependency</span>
          </div>
        </div>
      </footer>
    </div>
  );
};

export default App;
