// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

import {layoutTree} from "./layout";
import type {Transform, VisNode} from "./types";

interface Colors {
  accent: string;
  muted: string;
  ink: string;
  line: string;
  lineStrong: string;
  surface: string;
  codeBg: string;
  accentSoft: string;
}

export class AstCanvasRenderer {
  constructor(canvas: HTMLCanvasElement, container: HTMLElement) {
    this.canvas = canvas;
    this.container = container;
    this.ctx = canvas.getContext("2d")!;

    const cs = getComputedStyle(document.documentElement);
    this.colors = {
      accent: cs.getPropertyValue("--accent").trim(),
      muted: cs.getPropertyValue("--muted").trim(),
      ink: cs.getPropertyValue("--ink").trim(),
      line: cs.getPropertyValue("--line").trim(),
      lineStrong: cs.getPropertyValue("--line-strong").trim(),
      surface: cs.getPropertyValue("--surface").trim(),
      codeBg: cs.getPropertyValue("--code-bg").trim(),
      accentSoft: cs.getPropertyValue("--accent-soft").trim(),
    };

    this.bindEvents();
    this.resizeObserver = new ResizeObserver(() => {
      this.updateCanvasSize();
      this.render();
    });
    this.resizeObserver.observe(this.container);
  }

  destroy() {
    this.resizeObserver.disconnect();
  }

  refreshColors() {
    const cs = getComputedStyle(document.documentElement);
    this.colors = {
      accent: cs.getPropertyValue("--accent").trim(),
      muted: cs.getPropertyValue("--muted").trim(),
      ink: cs.getPropertyValue("--ink").trim(),
      line: cs.getPropertyValue("--line").trim(),
      lineStrong: cs.getPropertyValue("--line-strong").trim(),
      surface: cs.getPropertyValue("--surface").trim(),
      codeBg: cs.getPropertyValue("--code-bg").trim(),
      accentSoft: cs.getPropertyValue("--accent-soft").trim(),
    };
    this.render();
  }

  update(roots: VisNode[]) {
    this.updateCanvasSize();
    const layout = layoutTree(roots, this.ctx);
    this.tree = layout.roots;
    this.treeWidth = layout.width;
    this.treeHeight = layout.height;
    this.fitToView();
    this.render();
  }

  render() {
    const ctx = this.ctx;
    const w = this.displayWidth;
    const h = this.displayHeight;
    if (!w || !h) return;

    ctx.save();
    ctx.clearRect(0, 0, w, h);
    ctx.translate(this.transform.panX, this.transform.panY);
    ctx.scale(this.transform.zoom, this.transform.zoom);

    if (this.tree) {
      for (const root of this.tree) this.drawEdges(root);
      for (const root of this.tree) this.drawNodes(root);
    }

    ctx.restore();
  }

  private canvas: HTMLCanvasElement;
  private container: HTMLElement;
  private ctx: CanvasRenderingContext2D;
  private tree: VisNode[] | undefined = undefined;
  private treeWidth = 0;
  private treeHeight = 0;
  private transform: Transform = {panX: 0, panY: 0, zoom: 1.0};
  private hoverNode: VisNode | undefined = undefined;
  private dragging = false;
  private dragStart = {x: 0, y: 0};
  private dragPanStart = {x: 0, y: 0};
  private dragMoved = false;
  private colors: Colors;
  private resizeObserver: ResizeObserver;
  private displayWidth = 0;
  private displayHeight = 0;

  private bindEvents() {
    this.canvas.addEventListener("mousedown", (e) => this.onMouseDown(e));
    this.canvas.addEventListener("mousemove", (e) => this.onMouseMove(e));
    this.canvas.addEventListener("mouseup", (e) => this.onMouseUp(e));
    this.canvas.addEventListener("mouseleave", () => this.onMouseLeave());
    this.canvas.addEventListener("wheel", (e) => this.onWheel(e), {passive: false});
  }

  private updateCanvasSize() {
    const rect = this.container.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    this.canvas.width = rect.width * dpr;
    this.canvas.height = rect.height * dpr;
    this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    this.displayWidth = rect.width;
    this.displayHeight = rect.height;
  }

  private fitToView() {
    if (!this.tree || this.tree.length === 0) return;
    const pad = 40;
    const availW = this.displayWidth - pad * 2;
    const availH = this.displayHeight - pad * 2;
    if (availW <= 0 || availH <= 0) return;
    const scaleX = availW / this.treeWidth;
    const scaleY = availH / this.treeHeight;
    const zoomFitTree = Math.min(scaleX, scaleY);
    // Zoom so the root node occupies 10% of screen width
    const root = this.tree[0];
    const zoomRoot20pct = (0.2 * this.displayWidth) / root.w;
    // root-10% wins on big graphs (fit zoom is tiny), fit wins on small graphs (fit zoom is large)
    this.transform.zoom = Math.max(zoomFitTree, zoomRoot20pct);
    this.transform.panX = (this.displayWidth - this.treeWidth * this.transform.zoom) / 2;
    this.transform.panY = pad;
  }

  private drawEdges(node: VisNode) {
    if (node.collapsed) return;
    const ctx = this.ctx;
    ctx.strokeStyle = this.colors.lineStrong;
    ctx.lineWidth = 1;
    for (const child of node.children) {
      const fromX = node.x + node.w / 2;
      const fromY = node.y + node.h;
      const toX = child.x + child.w / 2;
      const toY = child.y;
      ctx.beginPath();
      ctx.moveTo(fromX, fromY);
      const midY = (fromY + toY) / 2;
      ctx.bezierCurveTo(fromX, midY, toX, midY, toX, toY);
      ctx.stroke();
      this.drawEdges(child);
    }
  }

  private drawNodes(node: VisNode) {
    const ctx = this.ctx;
    const isHover = node === this.hoverNode;
    const r = 6;

    ctx.beginPath();
    ctx.roundRect(node.x, node.y, node.w, node.h, r);
    ctx.fillStyle = isHover ? this.colors.accentSoft : this.colors.surface;
    ctx.fill();
    ctx.strokeStyle = node.collapsed ? this.colors.muted : this.colors.line;
    ctx.lineWidth = 1;
    if (node.collapsed) ctx.setLineDash([4, 3]);
    ctx.stroke();
    ctx.setLineDash([]);

    let displayLabel = node.label;
    if (node.fieldLabel) displayLabel = `${node.fieldLabel}: ${node.label}`;
    ctx.font = "bold 12px 'JetBrains Mono', monospace";
    ctx.fillStyle = this.colors.accent;
    ctx.textBaseline = "top";
    const textX = node.x + 10;
    let textY = node.y + 6;
    ctx.fillText(displayLabel, textX, textY);
    textY += 14;

    if (node.leafText && !node.collapsed) {
      ctx.font = "10px 'JetBrains Mono', monospace";
      ctx.fillStyle = this.colors.muted;
      if (node.kind !== "list") {
        const lines = node.leafText.split("\n");
        for (const line of lines) {
          ctx.fillText(line, textX, textY);
          textY += 14;
        }
      }
    }

    if (node.children.length > 0) {
      ctx.font = "bold 10px 'JetBrains Mono', monospace";
      ctx.fillStyle = this.colors.muted;
      const badge = node.collapsed ? "[+]" : "[-]";
      const badgeW = ctx.measureText(badge).width;
      ctx.fillText(badge, node.x + node.w - badgeW - 6, node.y + 6);
    }

    if (!node.collapsed) {
      for (const child of node.children) this.drawNodes(child);
    }
  }

  private canvasToTree(e: MouseEvent): {cx: number; cy: number; tx: number; ty: number} {
    const rect = this.canvas.getBoundingClientRect();
    const cx = e.clientX - rect.left;
    const cy = e.clientY - rect.top;
    const tx = (cx - this.transform.panX) / this.transform.zoom;
    const ty = (cy - this.transform.panY) / this.transform.zoom;
    return {cx, cy, tx, ty};
  }

  private hitTest(tx: number, ty: number): VisNode | undefined {
    if (!this.tree) return undefined;
    function check(node: VisNode): VisNode | undefined {
      if (tx >= node.x && tx <= node.x + node.w && ty >= node.y && ty <= node.y + node.h) {
        return node;
      }
      if (!node.collapsed) {
        for (const child of node.children) {
          const hit = check(child);
          if (hit) return hit;
        }
      }
      return undefined;
    }
    for (const root of this.tree) {
      const hit = check(root);
      if (hit) return hit;
    }
    return undefined;
  }

  private onMouseDown(e: MouseEvent) {
    this.dragging = true;
    this.dragMoved = false;
    this.dragStart = {x: e.clientX, y: e.clientY};
    this.dragPanStart = {x: this.transform.panX, y: this.transform.panY};
  }

  private onMouseMove(e: MouseEvent) {
    if (this.dragging) {
      const dx = e.clientX - this.dragStart.x;
      const dy = e.clientY - this.dragStart.y;
      if (Math.abs(dx) > 2 || Math.abs(dy) > 2) this.dragMoved = true;
      this.transform.panX = this.dragPanStart.x + dx;
      this.transform.panY = this.dragPanStart.y + dy;
      this.render();
    } else {
      const {tx, ty} = this.canvasToTree(e);
      const node = this.hitTest(tx, ty);
      if (node !== this.hoverNode) {
        this.hoverNode = node;
        this.canvas.style.cursor = node ? "pointer" : "move";
        this.render();
      }
    }
  }

  private onMouseUp(e: MouseEvent) {
    if (this.dragging && !this.dragMoved) {
      const {tx, ty} = this.canvasToTree(e);
      const node = this.hitTest(tx, ty);
      if (node && node.children.length > 0) {
        const oldScreenX = node.x * this.transform.zoom + this.transform.panX;
        const oldScreenY = node.y * this.transform.zoom + this.transform.panY;
        node.collapsed = !node.collapsed;
        const layout = layoutTree(this.tree!, this.ctx);
        this.tree = layout.roots;
        this.treeWidth = layout.width;
        this.treeHeight = layout.height;
        this.transform.panX = oldScreenX - node.x * this.transform.zoom;
        this.transform.panY = oldScreenY - node.y * this.transform.zoom;
        this.render();
      }
    }
    this.dragging = false;
    this.canvas.style.cursor = this.hoverNode ? "pointer" : "move";
  }

  private onMouseLeave() {
    this.dragging = false;
    if (this.hoverNode) {
      this.hoverNode = undefined;
      this.render();
    }
    this.canvas.style.cursor = "move";
  }

  zoomIn() {
    this.applyZoom(1.25);
  }

  zoomOut() {
    this.applyZoom(1 / 1.25);
  }

  private applyZoom(factor: number) {
    const cx = this.displayWidth / 2;
    const cy = this.displayHeight / 2;
    const oldZoom = this.transform.zoom;
    const newZoom = Math.max(0.15, Math.min(4.0, oldZoom * factor));
    this.transform.panX = cx - (cx - this.transform.panX) * (newZoom / oldZoom);
    this.transform.panY = cy - (cy - this.transform.panY) * (newZoom / oldZoom);
    this.transform.zoom = newZoom;
    this.render();
  }

  private onWheel(e: WheelEvent) {
    e.preventDefault();
    const {cx, cy} = this.canvasToTree(e);
    const oldZoom = this.transform.zoom;
    const factor = e.deltaY < 0 ? 1.1 : 1 / 1.1;
    const newZoom = Math.max(0.15, Math.min(4.0, oldZoom * factor));
    this.transform.panX = cx - (cx - this.transform.panX) * (newZoom / oldZoom);
    this.transform.panY = cy - (cy - this.transform.panY) * (newZoom / oldZoom);
    this.transform.zoom = newZoom;
    this.render();
  }

}
