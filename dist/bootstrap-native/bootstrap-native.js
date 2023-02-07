var BSN = function(exports) {
  "use strict";
  const e = {}, f = (t) => {
    const { type: c, currentTarget: i2 } = t;
    [...e[c]].forEach(([n, s]) => {
      i2 === n && [...s].forEach(([o, a]) => {
        o.apply(n, [t]), typeof a == "object" && a.once && r(n, c, o, a);
      });
    });
  }, E$1 = (t, c, i2, n) => {
    e[c] || (e[c] = /* @__PURE__ */ new Map());
    const s = e[c];
    s.has(t) || s.set(t, /* @__PURE__ */ new Map());
    const o = s.get(t), { size: a } = o;
    o.set(i2, n), a || t.addEventListener(c, f, n);
  }, r = (t, c, i2, n) => {
    const s = e[c], o = s && s.get(t), a = o && o.get(i2), d2 = a !== void 0 ? a : n;
    o && o.has(i2) && o.delete(i2), s && (!o || !o.size) && s.delete(t), (!s || !s.size) && delete e[c], (!o || !o.size) && t.removeEventListener(c, f, d2);
  }, g$1 = E$1, M$1 = r;
  const eventListener = /* @__PURE__ */ Object.freeze(/* @__PURE__ */ Object.defineProperty({
    __proto__: null,
    addListener: E$1,
    globalListener: f,
    off: M$1,
    on: g$1,
    registry: e,
    removeListener: r
  }, Symbol.toStringTag, { value: "Module" }));
  const ge = "aria-describedby", ve = "aria-expanded", be = "aria-hidden", we = "aria-modal", Ae = "aria-pressed", Se = "aria-selected", H = "DOMContentLoaded", _ = "focus", tt = "focusin", et = "focusout", ct = "keydown", at = "keyup", ut = "click", dt = "mousedown", ft = "hover", mt = "mouseenter", gt = "mouseleave", Mt = "pointerdown", kt = "pointermove", Tt = "pointerup", Lt = "resize", xt = "scroll", Bt = "touchstart", Le = "dragstart", Qe = "ArrowDown", qe = "ArrowUp", Ge = "ArrowLeft", je = "ArrowRight", $e = "Escape", Qt = "transitionDuration", qt = "transitionDelay", D = "transitionend", U = "transitionProperty", Gt = navigator.userAgentData, S = Gt, { userAgent: jt } = navigator, M = jt, I = /iPhone|iPad|iPod|Android/i;
  S ? S.brands.some((t) => I.test(t.brand)) : I.test(M);
  const z = /(iPhone|iPod|iPad)/, vn = S ? S.brands.some((t) => z.test(t.brand)) : z.test(M);
  M ? M.includes("Firefox") : false;
  const { head: N } = document;
  ["webkitPerspective", "perspective"].some((t) => t in N.style);
  const Jt = (t, e2, n, o) => {
    const s = o || false;
    t.addEventListener(e2, n, s);
  }, Kt = (t, e2, n, o) => {
    const s = o || false;
    t.removeEventListener(e2, n, s);
  }, Xt = (t, e2, n, o) => {
    const s = (c) => {
      (c.target === t || c.currentTarget === t) && (n.apply(t, [c]), Kt(t, e2, s, o));
    };
    Jt(t, e2, s, o);
  }, Yt = () => {
  };
  (() => {
    let t = false;
    try {
      const e2 = Object.defineProperty({}, "passive", {
        get: () => (t = true, t)
      });
      Xt(document, H, Yt, e2);
    } catch {
    }
    return t;
  })();
  ["webkitTransform", "transform"].some((t) => t in N.style);
  ["webkitAnimation", "animation"].some((t) => t in N.style);
  ["webkitTransition", "transition"].some((t) => t in N.style);
  const Zt = (t, e2) => t.getAttribute(e2), Nn = (t, e2) => t.hasAttribute(e2), F = (t, e2, n) => t.setAttribute(e2, n), Dn = (t, e2) => t.removeAttribute(e2), Ln = (t, ...e2) => {
    t.classList.add(...e2);
  }, On = (t, ...e2) => {
    t.classList.remove(...e2);
  }, In = (t, e2) => t.classList.contains(e2), E = (t) => t != null && typeof t == "object" || false, i = (t) => E(t) && typeof t.nodeType == "number" && [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11].some((e2) => t.nodeType === e2) || false, u = (t) => i(t) && t.nodeType === 1 || false, y = /* @__PURE__ */ new Map(), L = {
    set: (t, e2, n) => {
      if (!u(t))
        return;
      y.has(e2) || y.set(e2, /* @__PURE__ */ new Map()), y.get(e2).set(t, n);
    },
    getAllFor: (t) => y.get(t) || null,
    get: (t, e2) => {
      if (!u(t) || !e2)
        return null;
      const n = L.getAllFor(e2);
      return t && n && n.get(t) || null;
    },
    remove: (t, e2) => {
      const n = L.getAllFor(e2);
      !n || !u(t) || (n.delete(t), n.size === 0 && y.delete(e2));
    }
  }, Bn = (t, e2) => L.get(t, e2), g = (t) => typeof t == "string" || false, W = (t) => E(t) && t.constructor.name === "Window" || false, R = (t) => i(t) && t.nodeType === 9 || false, d = (t) => W(t) ? t.document : R(t) ? t : i(t) ? t.ownerDocument : window.document, h = (t) => Object.entries(t), $t = (t) => {
    if (!t)
      return;
    if (g(t))
      return d().createElement(t);
    const { tagName: e2 } = t, n = $t(e2);
    if (!n)
      return;
    const o = { ...t };
    return delete o.tagName, h(o).forEach(([s, c]) => {
      g(s) && g(c) && F(n, s, c);
    }), n;
  }, Q = (t, e2) => t.dispatchEvent(e2), v = (t, e2) => {
    const n = getComputedStyle(t), o = e2.replace("webkit", "Webkit").replace(/([A-Z])/g, "-$1").toLowerCase();
    return n.getPropertyValue(o);
  }, ne = (t) => {
    const e2 = v(t, U), n = v(t, qt), o = n.includes("ms") ? 1 : 1e3, s = e2 && e2 !== "none" ? parseFloat(n) * o : 0;
    return Number.isNaN(s) ? 0 : s;
  }, oe = (t) => {
    const e2 = v(t, U), n = v(t, Qt), o = n.includes("ms") ? 1 : 1e3, s = e2 && e2 !== "none" ? parseFloat(n) * o : 0;
    return Number.isNaN(s) ? 0 : s;
  }, Un = (t, e2) => {
    let n = 0;
    const o = new Event(D), s = oe(t), c = ne(t);
    if (s) {
      const a = (l) => {
        l.target === t && (e2.apply(t, [l]), t.removeEventListener(D, a), n = 1);
      };
      t.addEventListener(D, a), setTimeout(() => {
        n || Q(t, o);
      }, s + c + 17);
    } else
      e2.apply(t, [o]);
  }, Rn = (t, e2) => t.focus(e2), x = (t) => ["true", true].includes(t) ? true : ["false", false].includes(t) ? false : ["null", "", null, void 0].includes(t) ? null : t !== "" && !Number.isNaN(+t) ? +t : t, se = (t) => t.toLowerCase(), Qn = (t, e2, n, o) => {
    const s = { ...n }, c = { ...t.dataset }, a = { ...e2 }, l = {}, p = "title";
    return h(c).forEach(([r2, f2]) => {
      const A = o && typeof r2 == "string" && r2.includes(o) ? r2.replace(o, "").replace(/[A-Z]/g, (G) => se(G)) : r2;
      l[A] = x(f2);
    }), h(s).forEach(([r2, f2]) => {
      s[r2] = x(f2);
    }), h(e2).forEach(([r2, f2]) => {
      r2 in s ? a[r2] = s[r2] : r2 in l ? a[r2] = l[r2] : a[r2] = r2 === p ? Zt(t, p) : f2;
    }), a;
  }, q = (t, ...e2) => Object.assign(t, ...e2), Gn = (t) => Object.keys(t), jn = (t) => Object.values(t), Jn = (t, e2) => {
    const n = new CustomEvent(t, {
      cancelable: true,
      bubbles: true
    });
    return E(e2) && q(n, e2), n;
  }, Kn = { passive: true }, Xn = (t) => t.offsetHeight, Yn = (t, e2) => {
    h(e2).forEach(([n, o]) => {
      if (o && g(n) && n.includes("--"))
        t.style.setProperty(n, o);
      else {
        const s = {};
        s[n] = o, q(t.style, s);
      }
    });
  }, O = (t) => E(t) && t.constructor.name === "Map" || false, ce = (t) => typeof t == "number" || false, m = /* @__PURE__ */ new Map(), Zn = {
    set: (t, e2, n, o) => {
      !u(t) || (o && o.length ? (m.has(t) || m.set(t, /* @__PURE__ */ new Map()), m.get(t).set(o, setTimeout(e2, n))) : m.set(t, setTimeout(e2, n)));
    },
    get: (t, e2) => {
      if (!u(t))
        return null;
      const n = m.get(t);
      return e2 && n && O(n) ? n.get(e2) || null : ce(n) ? n : null;
    },
    clear: (t, e2) => {
      if (!u(t))
        return;
      const n = m.get(t);
      e2 && e2.length && O(n) ? (clearTimeout(n.get(e2)), n.delete(e2), n.size === 0 && m.delete(t)) : (clearTimeout(n), m.delete(t));
    }
  }, w = (t, e2) => {
    const { width: n, height: o, top: s, right: c, bottom: a, left: l } = t.getBoundingClientRect();
    let p = 1, r2 = 1;
    if (e2 && u(t)) {
      const { offsetWidth: f2, offsetHeight: A } = t;
      p = f2 > 0 ? Math.round(n) / f2 : 1, r2 = A > 0 ? Math.round(o) / A : 1;
    }
    return {
      width: n / p,
      height: o / r2,
      top: s / r2,
      right: c / p,
      bottom: a / r2,
      left: l / p,
      x: l / p,
      y: s / r2
    };
  }, _n = (t) => d(t).body, k = (t) => d(t).documentElement, re = (t) => i(t) && t.constructor.name === "ShadowRoot" || false, no = (t) => t.nodeName === "HTML" ? t : u(t) && t.assignedSlot || i(t) && t.parentNode || re(t) && t.host || k(t);
  let V = 0, B = 0;
  const b = /* @__PURE__ */ new Map(), ie = (t, e2) => {
    let n = e2 ? V : B;
    if (e2) {
      const o = ie(t), s = b.get(o) || /* @__PURE__ */ new Map();
      b.has(o) || b.set(o, s), O(s) && !s.has(e2) ? (s.set(e2, n), V += 1) : n = s.get(e2);
    } else {
      const o = t.id || t;
      b.has(o) ? n = b.get(o) : (b.set(o, n), B += 1);
    }
    return n;
  }, so = (t) => {
    var e2;
    return t ? R(t) ? t.defaultView : i(t) ? (e2 = t == null ? void 0 : t.ownerDocument) == null ? void 0 : e2.defaultView : t : window;
  }, ue = (t) => Array.isArray(t) || false, ao = (t) => {
    if (!i(t))
      return false;
    const { top: e2, bottom: n } = w(t), { clientHeight: o } = k(t);
    return e2 <= o && n >= 0;
  }, lo = (t) => typeof t == "function" || false, Eo = (t) => E(t) && t.constructor.name === "NodeList" || false, bo = (t) => k(t).dir === "rtl", yo = (t) => i(t) && ["TABLE", "TD", "TH"].includes(t.nodeName) || false, de = (t, e2) => t ? t.closest(e2) || de(t.getRootNode().host, e2) : null, wo = (t, e2) => u(t) ? t : (i(e2) ? e2 : d()).querySelector(t), pe = (t, e2) => (i(e2) ? e2 : d()).getElementsByTagName(t), Mo = (t, e2) => (i(e2) ? e2 : d()).querySelectorAll(t), No = (t, e2) => (e2 && i(e2) ? e2 : d()).getElementsByClassName(
    t
  ), ko = (t, e2) => t.matches(e2);
  const fadeClass = "fade";
  const showClass = "show";
  const dataBsDismiss = "data-bs-dismiss";
  const alertString = "alert";
  const alertComponent = "Alert";
  const version = "5.0.0-alpha5";
  const Version = version;
  class BaseComponent {
    element;
    options;
    /**
     * @param target `HTMLElement` or selector string
     * @param config component instance options
     */
    constructor(target, config) {
      const element = wo(target);
      if (!element) {
        if (g(target)) {
          throw Error(`${this.name} Error: "${target}" is not a valid selector.`);
        } else {
          throw Error(`${this.name} Error: your target is not an instance of HTMLElement.`);
        }
      }
      const prevInstance = L.get(element, this.name);
      if (prevInstance)
        prevInstance.dispose();
      this.element = element;
      if (this.defaults && Gn(this.defaults).length) {
        this.options = Qn(element, this.defaults, config || {}, "bs");
      }
      L.set(element, this.name, this);
    }
    /* istanbul ignore next */
    get version() {
      return Version;
    }
    /* istanbul ignore next */
    get name() {
      return "BaseComponent";
    }
    /* istanbul ignore next */
    get defaults() {
      return {};
    }
    /**
     * Removes component from target element;
     */
    dispose() {
      L.remove(this.element, this.name);
      Gn(this).forEach((prop) => {
        delete this[prop];
      });
    }
  }
  const alertSelector = `.${alertString}`;
  const alertDismissSelector = `[${dataBsDismiss}="${alertString}"]`;
  const getAlertInstance = (element) => Bn(element, alertComponent);
  const alertInitCallback = (element) => new Alert(element);
  const closeAlertEvent = Jn(`close.bs.${alertString}`);
  const closedAlertEvent = Jn(`closed.bs.${alertString}`);
  const alertTransitionEnd = (that) => {
    const { element } = that;
    toggleAlertHandler(that);
    Q(element, closedAlertEvent);
    that.dispose();
    element.remove();
  };
  const toggleAlertHandler = (that, add) => {
    const action = add ? E$1 : r;
    const { dismiss } = that;
    if (dismiss)
      action(dismiss, ut, that.close);
  };
  class Alert extends BaseComponent {
    static selector = alertSelector;
    static init = alertInitCallback;
    static getInstance = getAlertInstance;
    dismiss;
    constructor(target) {
      super(target);
      this.dismiss = wo(alertDismissSelector, this.element);
      toggleAlertHandler(this, true);
    }
    /** Returns component name string. */
    get name() {
      return alertComponent;
    }
    // ALERT PUBLIC METHODS
    // ====================
    /**
     * Public method that hides the `.alert` element from the user,
     * disposes the instance once animation is complete, then
     * removes the element from the DOM.
     *
     * @param e the `click` event
     */
    close(e2) {
      const self = e2 ? getAlertInstance(de(e2.target, alertSelector)) : this;
      const { element } = self;
      if (element && In(element, showClass)) {
        Q(element, closeAlertEvent);
        if (closeAlertEvent.defaultPrevented)
          return;
        On(element, showClass);
        if (In(element, fadeClass)) {
          Un(element, () => alertTransitionEnd(self));
        } else
          alertTransitionEnd(self);
      }
    }
    /** Remove the component from target element. */
    dispose() {
      toggleAlertHandler(this);
      super.dispose();
    }
  }
  const activeClass = "active";
  const dataBsToggle = "data-bs-toggle";
  const buttonString = "button";
  const buttonComponent = "Button";
  const buttonSelector = `[${dataBsToggle}="${buttonString}"]`;
  const getButtonInstance = (element) => Bn(element, buttonComponent);
  const buttonInitCallback = (element) => new Button(element);
  const toggleButtonHandler = (self, add) => {
    const action = add ? E$1 : r;
    action(self.element, ut, self.toggle);
  };
  class Button extends BaseComponent {
    static selector = buttonSelector;
    static init = buttonInitCallback;
    static getInstance = getButtonInstance;
    isActive = false;
    /**
     * @param target usually a `.btn` element
     */
    constructor(target) {
      super(target);
      const { element } = this;
      this.isActive = In(element, activeClass);
      F(element, Ae, String(!!this.isActive));
      toggleButtonHandler(this, true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return buttonComponent;
    }
    // BUTTON PUBLIC METHODS
    // =====================
    /**
     * Toggles the state of the target button.
     *
     * @param e usually `click` Event object
     */
    toggle(e2) {
      if (e2)
        e2.preventDefault();
      const self = e2 ? getButtonInstance(e2.target) : this;
      if (!self.element)
        return;
      const { element, isActive } = self;
      if (In(element, "disabled"))
        return;
      const action = isActive ? On : Ln;
      action(element, activeClass);
      F(element, Ae, isActive ? "false" : "true");
      self.isActive = In(element, activeClass);
    }
    /** Removes the `Button` component from the target element. */
    dispose() {
      toggleButtonHandler(this);
      super.dispose();
    }
  }
  const dataBsTarget = "data-bs-target";
  const carouselString = "carousel";
  const carouselComponent = "Carousel";
  const dataBsParent = "data-bs-parent";
  const dataBsContainer = "data-bs-container";
  const getTargetElement = (element) => {
    const targetAttr = [dataBsTarget, dataBsParent, dataBsContainer, "href"];
    const doc = d(element);
    return targetAttr.map((att) => {
      const attValue = Zt(element, att);
      if (attValue) {
        return att === dataBsParent ? de(element, attValue) : wo(attValue, doc);
      }
      return null;
    }).filter((x2) => x2)[0];
  };
  const carouselSelector = `[data-bs-ride="${carouselString}"]`;
  const carouselItem = `${carouselString}-item`;
  const dataBsSlideTo = "data-bs-slide-to";
  const dataBsSlide = "data-bs-slide";
  const pausedClass = "paused";
  const carouselDefaults = {
    pause: "hover",
    keyboard: false,
    touch: true,
    interval: 5e3
  };
  const getCarouselInstance = (element) => Bn(element, carouselComponent);
  const carouselInitCallback = (element) => new Carousel(element);
  let startX = 0;
  let currentX = 0;
  let endX = 0;
  const carouselSlideEvent = Jn(`slide.bs.${carouselString}`);
  const carouselSlidEvent = Jn(`slid.bs.${carouselString}`);
  const carouselTransitionEndHandler = (self) => {
    const { index, direction, element, slides, options } = self;
    if (self.isAnimating && getCarouselInstance(element)) {
      const activeItem = getActiveIndex(self);
      const orientation = direction === "left" ? "next" : "prev";
      const directionClass = direction === "left" ? "start" : "end";
      Ln(slides[index], activeClass);
      On(slides[index], `${carouselItem}-${orientation}`);
      On(slides[index], `${carouselItem}-${directionClass}`);
      On(slides[activeItem], activeClass);
      On(slides[activeItem], `${carouselItem}-${directionClass}`);
      Q(element, carouselSlidEvent);
      Zn.clear(element, dataBsSlide);
      if (!d(element).hidden && options.interval && !self.isPaused) {
        self.cycle();
      }
    }
  };
  function carouselPauseHandler() {
    const self = getCarouselInstance(this);
    if (self && !self.isPaused && !Zn.get(this, pausedClass)) {
      Ln(this, pausedClass);
    }
  }
  function carouselResumeHandler() {
    const self = getCarouselInstance(this);
    if (self && self.isPaused && !Zn.get(this, pausedClass)) {
      self.cycle();
    }
  }
  function carouselIndicatorHandler(e2) {
    e2.preventDefault();
    const element = de(this, carouselSelector) || getTargetElement(this);
    const self = getCarouselInstance(element);
    if (!self || self.isAnimating)
      return;
    const newIndex = +(Zt(this, dataBsSlideTo) || /* istanbul ignore next */
    0);
    if (this && !In(this, activeClass) && // event target is not active
    !Number.isNaN(newIndex)) {
      self.to(newIndex);
    }
  }
  function carouselControlsHandler(e2) {
    e2.preventDefault();
    const element = de(this, carouselSelector) || getTargetElement(this);
    const self = getCarouselInstance(element);
    if (!self || self.isAnimating)
      return;
    const orientation = Zt(this, dataBsSlide);
    if (orientation === "next") {
      self.next();
    } else if (orientation === "prev") {
      self.prev();
    }
  }
  const carouselKeyHandler = ({ code, target }) => {
    const doc = d(target);
    const [element] = [...Mo(carouselSelector, doc)].filter((x2) => ao(x2));
    const self = getCarouselInstance(element);
    if (!self || self.isAnimating || /textarea|input/i.test(target.nodeName))
      return;
    const RTL = bo(element);
    const arrowKeyNext = !RTL ? je : Ge;
    const arrowKeyPrev = !RTL ? Ge : je;
    if (code === arrowKeyPrev)
      self.prev();
    else if (code === arrowKeyNext)
      self.next();
  };
  function carouselDragHandler(e2) {
    const { target } = e2;
    const self = getCarouselInstance(this);
    if (self && self.isTouch && (self.indicator && !self.indicator.contains(target) || !self.controls.includes(target))) {
      e2.stopImmediatePropagation();
      e2.stopPropagation();
      e2.preventDefault();
    }
  }
  function carouselPointerDownHandler(e2) {
    const { target } = e2;
    const self = getCarouselInstance(this);
    if (!self || self.isAnimating || self.isTouch) {
      return;
    }
    const { controls, indicators } = self;
    if ([...controls, ...indicators].some((el) => el === target || el.contains(target))) {
      return;
    }
    startX = e2.pageX;
    if (this.contains(target)) {
      self.isTouch = true;
      toggleCarouselTouchHandlers(self, true);
    }
  }
  const carouselPointerMoveHandler = (e2) => {
    currentX = e2.pageX;
  };
  const carouselPointerUpHandler = (e2) => {
    const { target } = e2;
    const doc = d(target);
    const self = [...Mo(carouselSelector, doc)].map((c) => getCarouselInstance(c)).find((i2) => i2.isTouch);
    if (!self) {
      return;
    }
    const { element, index } = self;
    const RTL = bo(element);
    self.isTouch = false;
    toggleCarouselTouchHandlers(self);
    if (doc.getSelection()?.toString().length) {
      startX = 0;
      currentX = 0;
      endX = 0;
      return;
    }
    endX = e2.pageX;
    if (!element.contains(target) || Math.abs(startX - endX) < 120) {
      startX = 0;
      currentX = 0;
      endX = 0;
      return;
    }
    if (currentX < startX) {
      self.to(index + (RTL ? -1 : 1));
    } else if (currentX > startX) {
      self.to(index + (RTL ? 1 : -1));
    }
    startX = 0;
    currentX = 0;
    endX = 0;
  };
  const activateCarouselIndicator = (self, pageIndex) => {
    const { indicators } = self;
    [...indicators].forEach((x2) => On(x2, activeClass));
    if (self.indicators[pageIndex])
      Ln(indicators[pageIndex], activeClass);
  };
  const toggleCarouselTouchHandlers = (self, add) => {
    const { element } = self;
    const action = add ? E$1 : r;
    action(d(element), kt, carouselPointerMoveHandler, Kn);
    action(d(element), Tt, carouselPointerUpHandler, Kn);
  };
  const toggleCarouselHandlers = (self, add) => {
    const { element, options, slides, controls, indicators } = self;
    const { touch, pause, interval, keyboard } = options;
    const action = add ? E$1 : r;
    if (pause && interval) {
      action(element, mt, carouselPauseHandler);
      action(element, gt, carouselResumeHandler);
    }
    if (touch && slides.length > 2) {
      action(element, Mt, carouselPointerDownHandler, Kn);
      action(element, Bt, carouselDragHandler, { passive: false });
      action(element, Le, carouselDragHandler, { passive: false });
    }
    if (controls.length) {
      controls.forEach((arrow) => {
        if (arrow)
          action(arrow, ut, carouselControlsHandler);
      });
    }
    if (indicators.length) {
      indicators.forEach((indicator) => {
        action(indicator, ut, carouselIndicatorHandler);
      });
    }
    if (keyboard)
      action(d(element), ct, carouselKeyHandler);
  };
  const getActiveIndex = (self) => {
    const { slides, element } = self;
    const activeItem = wo(`.${carouselItem}.${activeClass}`, element);
    return u(activeItem) ? [...slides].indexOf(activeItem) : -1;
  };
  class Carousel extends BaseComponent {
    static selector = carouselSelector;
    static init = carouselInitCallback;
    static getInstance = getCarouselInstance;
    /**
     * @param target mostly a `.carousel` element
     * @param config instance options
     */
    constructor(target, config) {
      super(target, config);
      const { element } = this;
      this.direction = bo(element) ? "right" : "left";
      this.index = 0;
      this.isTouch = false;
      this.slides = No(carouselItem, element);
      const { slides } = this;
      if (slides.length < 2) {
        return;
      }
      const doc = d(element);
      this.controls = [
        ...Mo(`[${dataBsSlide}]`, element),
        ...Mo(`[${dataBsSlide}][${dataBsTarget}="#${element.id}"]`, doc)
      ];
      this.indicator = wo(`.${carouselString}-indicators`, element);
      this.indicators = [
        ...this.indicator ? Mo(`[${dataBsSlideTo}]`, this.indicator) : [],
        ...Mo(`[${dataBsSlideTo}][${dataBsTarget}="#${element.id}"]`, doc)
      ];
      const { options } = this;
      this.options.interval = options.interval === true ? carouselDefaults.interval : options.interval;
      if (getActiveIndex(this) < 0) {
        Ln(slides[0], activeClass);
        if (this.indicators.length)
          activateCarouselIndicator(this, 0);
      }
      toggleCarouselHandlers(this, true);
      if (options.interval)
        this.cycle();
    }
    /**
     * Returns component name string.
     */
    get name() {
      return carouselComponent;
    }
    /**
     * Returns component default options.
     */
    get defaults() {
      return carouselDefaults;
    }
    /**
     * Check if instance is paused.
     */
    get isPaused() {
      return In(this.element, pausedClass);
    }
    /**
     * Check if instance is animating.
     */
    get isAnimating() {
      return wo(`.${carouselItem}-next,.${carouselItem}-prev`, this.element) !== null;
    }
    // CAROUSEL PUBLIC METHODS
    // =======================
    /** Slide automatically through items. */
    cycle() {
      const { element, options, isPaused, index } = this;
      Zn.clear(element, carouselString);
      if (isPaused) {
        Zn.clear(element, pausedClass);
        On(element, pausedClass);
      }
      Zn.set(
        element,
        () => {
          if (this.element && !this.isPaused && !this.isTouch && ao(element)) {
            this.to(index + 1);
          }
        },
        options.interval,
        carouselString
      );
    }
    /** Pause the automatic cycle. */
    pause() {
      const { element, options } = this;
      if (!this.isPaused && options.interval) {
        Ln(element, pausedClass);
        Zn.set(
          element,
          () => {
          },
          1,
          pausedClass
        );
      }
    }
    /** Slide to the next item. */
    next() {
      if (!this.isAnimating) {
        this.to(this.index + 1);
      }
    }
    /** Slide to the previous item. */
    prev() {
      if (!this.isAnimating) {
        this.to(this.index - 1);
      }
    }
    /**
     * Jump to the item with the `idx` index.
     *
     * @param idx the index of the item to jump to
     */
    to(idx) {
      const { element, slides, options } = this;
      const activeItem = getActiveIndex(this);
      const RTL = bo(element);
      let next = idx;
      if (this.isAnimating || activeItem === next || Zn.get(element, dataBsSlide))
        return;
      if (activeItem < next || activeItem === 0 && next === slides.length - 1) {
        this.direction = RTL ? "right" : "left";
      } else if (activeItem > next || activeItem === slides.length - 1 && next === 0) {
        this.direction = RTL ? "left" : "right";
      }
      const { direction } = this;
      if (next < 0) {
        next = slides.length - 1;
      } else if (next >= slides.length) {
        next = 0;
      }
      const orientation = direction === "left" ? "next" : "prev";
      const directionClass = direction === "left" ? "start" : "end";
      const eventProperties = {
        relatedTarget: slides[next],
        from: activeItem,
        to: next,
        direction
      };
      q(carouselSlideEvent, eventProperties);
      q(carouselSlidEvent, eventProperties);
      Q(element, carouselSlideEvent);
      if (carouselSlideEvent.defaultPrevented)
        return;
      this.index = next;
      activateCarouselIndicator(this, next);
      if (oe(slides[next]) && In(element, "slide")) {
        Zn.set(
          element,
          () => {
            Ln(slides[next], `${carouselItem}-${orientation}`);
            Xn(slides[next]);
            Ln(slides[next], `${carouselItem}-${directionClass}`);
            Ln(slides[activeItem], `${carouselItem}-${directionClass}`);
            Un(slides[next], () => carouselTransitionEndHandler(this));
          },
          0,
          dataBsSlide
        );
      } else {
        Ln(slides[next], activeClass);
        On(slides[activeItem], activeClass);
        Zn.set(
          element,
          () => {
            Zn.clear(element, dataBsSlide);
            if (element && options.interval && !this.isPaused) {
              this.cycle();
            }
            Q(element, carouselSlidEvent);
          },
          0,
          dataBsSlide
        );
      }
    }
    /** Remove `Carousel` component from target. */
    dispose() {
      const { slides } = this;
      const itemClasses = ["start", "end", "prev", "next"];
      [...slides].forEach((slide, idx) => {
        if (In(slide, activeClass))
          activateCarouselIndicator(this, idx);
        itemClasses.forEach((c) => On(slide, `${carouselItem}-${c}`));
      });
      toggleCarouselHandlers(this);
      super.dispose();
    }
  }
  const collapsingClass = "collapsing";
  const collapseString = "collapse";
  const collapseComponent = "Collapse";
  const collapseSelector = `.${collapseString}`;
  const collapseToggleSelector = `[${dataBsToggle}="${collapseString}"]`;
  const collapseDefaults = { parent: null };
  const getCollapseInstance = (element) => Bn(element, collapseComponent);
  const collapseInitCallback = (element) => new Collapse(element);
  const showCollapseEvent = Jn(`show.bs.${collapseString}`);
  const shownCollapseEvent = Jn(`shown.bs.${collapseString}`);
  const hideCollapseEvent = Jn(`hide.bs.${collapseString}`);
  const hiddenCollapseEvent = Jn(`hidden.bs.${collapseString}`);
  const expandCollapse = (self) => {
    const { element, parent, triggers } = self;
    Q(element, showCollapseEvent);
    if (showCollapseEvent.defaultPrevented)
      return;
    Zn.set(element, Yt, 17);
    if (parent)
      Zn.set(parent, Yt, 17);
    Ln(element, collapsingClass);
    On(element, collapseString);
    Yn(element, { height: `${element.scrollHeight}px` });
    Un(element, () => {
      Zn.clear(element);
      if (parent)
        Zn.clear(parent);
      triggers.forEach((btn) => F(btn, ve, "true"));
      On(element, collapsingClass);
      Ln(element, collapseString);
      Ln(element, showClass);
      Yn(element, { height: "" });
      Q(element, shownCollapseEvent);
    });
  };
  const collapseContent = (self) => {
    const { element, parent, triggers } = self;
    Q(element, hideCollapseEvent);
    if (hideCollapseEvent.defaultPrevented)
      return;
    Zn.set(element, Yt, 17);
    if (parent)
      Zn.set(parent, Yt, 17);
    Yn(element, { height: `${element.scrollHeight}px` });
    On(element, collapseString);
    On(element, showClass);
    Ln(element, collapsingClass);
    Xn(element);
    Yn(element, { height: "0px" });
    Un(element, () => {
      Zn.clear(element);
      if (parent)
        Zn.clear(parent);
      triggers.forEach((btn) => F(btn, ve, "false"));
      On(element, collapsingClass);
      Ln(element, collapseString);
      Yn(element, { height: "" });
      Q(element, hiddenCollapseEvent);
    });
  };
  const toggleCollapseHandler = (self, add) => {
    const action = add ? E$1 : r;
    const { triggers } = self;
    if (triggers.length) {
      triggers.forEach((btn) => action(btn, ut, collapseClickHandler));
    }
  };
  const collapseClickHandler = (e2) => {
    const { target } = e2;
    const trigger = target && de(target, collapseToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getCollapseInstance(element);
    if (self)
      self.toggle();
    if (trigger && trigger.tagName === "A")
      e2.preventDefault();
  };
  class Collapse extends BaseComponent {
    static selector = collapseSelector;
    static init = collapseInitCallback;
    static getInstance = getCollapseInstance;
    /**
     * @param target and `Element` that matches the selector
     * @param config instance options
     */
    constructor(target, config) {
      super(target, config);
      const { element, options } = this;
      const doc = d(element);
      this.triggers = [...Mo(collapseToggleSelector, doc)].filter((btn) => getTargetElement(btn) === element);
      this.parent = wo(options.parent, doc) || getTargetElement(element) || null;
      this.parent = getTargetElement(element) || null;
      toggleCollapseHandler(this, true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return collapseComponent;
    }
    /**
     * Returns component default options.
     */
    get defaults() {
      return collapseDefaults;
    }
    // COLLAPSE PUBLIC METHODS
    // =======================
    /** Toggles the visibility of the collapse. */
    toggle() {
      if (!In(this.element, showClass))
        this.show();
      else
        this.hide();
    }
    /** Hides the collapse. */
    hide() {
      const { triggers, element } = this;
      if (Zn.get(element))
        return;
      collapseContent(this);
      if (triggers.length) {
        triggers.forEach((btn) => Ln(btn, `${collapseString}d`));
      }
    }
    /** Shows the collapse. */
    show() {
      const { element, parent, triggers } = this;
      let activeCollapse;
      let activeCollapseInstance;
      if (parent) {
        activeCollapse = [...Mo(`.${collapseString}.${showClass}`, parent)].find(
          (i2) => getCollapseInstance(i2)
        );
        activeCollapseInstance = activeCollapse && getCollapseInstance(activeCollapse);
      }
      if ((!parent || !Zn.get(parent)) && !Zn.get(element)) {
        if (activeCollapseInstance && activeCollapse !== element) {
          collapseContent(activeCollapseInstance);
          activeCollapseInstance.triggers.forEach((btn) => {
            Ln(btn, `${collapseString}d`);
          });
        }
        expandCollapse(this);
        if (triggers.length) {
          triggers.forEach((btn) => On(btn, `${collapseString}d`));
        }
      }
    }
    /** Remove the `Collapse` component from the target `Element`. */
    dispose() {
      toggleCollapseHandler(this);
      super.dispose();
    }
  }
  const dropdownMenuClasses = ["dropdown", "dropup", "dropstart", "dropend"];
  const dropdownComponent = "Dropdown";
  const dropdownMenuClass = "dropdown-menu";
  const isEmptyAnchor = (element) => {
    const parentAnchor = de(element, "A");
    return element.tagName === "A" && // anchor href starts with #
    Nn(element, "href") && element.href.slice(-1) === "#" || // OR a child of an anchor with href starts with #
    parentAnchor && Nn(parentAnchor, "href") && parentAnchor.href.slice(-1) === "#";
  };
  const [dropdownString, dropupString, dropstartString, dropendString] = dropdownMenuClasses;
  const dropdownSelector = `[${dataBsToggle}="${dropdownString}"],[${dataBsToggle}="${dropupString}"],[${dataBsToggle}="${dropendString}"],[${dataBsToggle}="${dropstartString}"]`;
  const getDropdownInstance = (element) => Bn(element, dropdownComponent);
  const dropdownInitCallback = (element) => new Dropdown(element);
  const dropdownMenuEndClass = `${dropdownMenuClass}-end`;
  const verticalClass = [dropdownString, dropupString];
  const horizontalClass = [dropstartString, dropendString];
  const menuFocusTags = ["A", "BUTTON"];
  const dropdownDefaults = {
    offset: 5,
    // [number] 5(px)
    display: "dynamic"
    // [dynamic|static]
  };
  const showDropdownEvent = Jn(`show.bs.${dropdownString}`);
  const shownDropdownEvent = Jn(`shown.bs.${dropdownString}`);
  const hideDropdownEvent = Jn(`hide.bs.${dropdownString}`);
  const hiddenDropdownEvent = Jn(`hidden.bs.${dropdownString}`);
  const updatedDropdownEvent = Jn(`updated.bs.${dropdownString}`);
  const styleDropdown = (self) => {
    const { element, menu, parentElement, options } = self;
    const { offset } = options;
    if (v(menu, "position") === "static")
      return;
    const RTL = bo(element);
    const menuEnd = In(menu, dropdownMenuEndClass);
    const resetProps = ["margin", "top", "bottom", "left", "right"];
    resetProps.forEach((p) => {
      const style = {};
      style[p] = "";
      Yn(menu, style);
    });
    let positionClass = dropdownMenuClasses.find((c) => In(parentElement, c)) || /* istanbul ignore next: fallback position */
    dropdownString;
    const dropdownMargin = {
      dropdown: [offset, 0, 0],
      dropup: [0, 0, offset],
      dropstart: RTL ? [-1, 0, 0, offset] : [-1, offset, 0],
      dropend: RTL ? [-1, offset, 0] : [-1, 0, 0, offset]
    };
    const dropdownPosition = {
      dropdown: { top: "100%" },
      dropup: { top: "auto", bottom: "100%" },
      dropstart: RTL ? { left: "100%", right: "auto" } : { left: "auto", right: "100%" },
      dropend: RTL ? { left: "auto", right: "100%" } : { left: "100%", right: "auto" },
      menuStart: RTL ? { right: "0", left: "auto" } : { right: "auto", left: "0" },
      menuEnd: RTL ? { right: "auto", left: "0" } : { right: "0", left: "auto" }
    };
    const { offsetWidth: menuWidth, offsetHeight: menuHeight } = menu;
    const { clientWidth, clientHeight } = k(element);
    const { left: targetLeft, top: targetTop, width: targetWidth, height: targetHeight } = w(element);
    const leftFullExceed = targetLeft - menuWidth - offset < 0;
    const rightFullExceed = targetLeft + menuWidth + targetWidth + offset >= clientWidth;
    const bottomExceed = targetTop + menuHeight + offset >= clientHeight;
    const bottomFullExceed = targetTop + menuHeight + targetHeight + offset >= clientHeight;
    const topExceed = targetTop - menuHeight - offset < 0;
    const leftExceed = (!RTL && menuEnd || RTL && !menuEnd) && targetLeft + targetWidth - menuWidth < 0;
    const rightExceed = (RTL && menuEnd || !RTL && !menuEnd) && targetLeft + menuWidth >= clientWidth;
    if (horizontalClass.includes(positionClass) && leftFullExceed && rightFullExceed) {
      positionClass = dropdownString;
    }
    if (positionClass === dropstartString && (!RTL ? leftFullExceed : rightFullExceed)) {
      positionClass = dropendString;
    }
    if (positionClass === dropendString && (RTL ? leftFullExceed : rightFullExceed)) {
      positionClass = dropstartString;
    }
    if (positionClass === dropupString && topExceed && !bottomFullExceed) {
      positionClass = dropdownString;
    }
    if (positionClass === dropdownString && bottomFullExceed && !topExceed) {
      positionClass = dropupString;
    }
    if (horizontalClass.includes(positionClass) && bottomExceed) {
      q(dropdownPosition[positionClass], {
        top: "auto",
        bottom: 0
      });
    }
    if (verticalClass.includes(positionClass) && (leftExceed || rightExceed)) {
      let posAjust = { left: "auto", right: "auto" };
      if (!leftExceed && rightExceed && !RTL)
        posAjust = { left: "auto", right: 0 };
      if (leftExceed && !rightExceed && RTL)
        posAjust = { left: 0, right: "auto" };
      if (posAjust)
        q(dropdownPosition[positionClass], posAjust);
    }
    const margins = dropdownMargin[positionClass];
    Yn(menu, {
      ...dropdownPosition[positionClass],
      margin: `${margins.map((x2) => x2 ? `${x2}px` : x2).join(" ")}`
    });
    if (verticalClass.includes(positionClass) && menuEnd) {
      if (menuEnd) {
        const endAdjust = !RTL && leftExceed || RTL && rightExceed ? "menuStart" : (
          /* istanbul ignore next */
          "menuEnd"
        );
        Yn(menu, dropdownPosition[endAdjust]);
      }
    }
    Q(parentElement, updatedDropdownEvent);
  };
  const getMenuItems = (menu) => {
    return [...menu.children].map((c) => {
      if (c && menuFocusTags.includes(c.tagName))
        return c;
      const { firstElementChild } = c;
      if (firstElementChild && menuFocusTags.includes(firstElementChild.tagName)) {
        return firstElementChild;
      }
      return null;
    }).filter((c) => c);
  };
  const toggleDropdownDismiss = (self) => {
    const { element, options } = self;
    const action = self.open ? E$1 : r;
    const doc = d(element);
    action(doc, ut, dropdownDismissHandler);
    action(doc, _, dropdownDismissHandler);
    action(doc, ct, dropdownPreventScroll);
    action(doc, at, dropdownKeyHandler);
    if (options.display === "dynamic") {
      [xt, Lt].forEach((ev) => {
        action(so(element), ev, dropdownLayoutHandler, Kn);
      });
    }
  };
  const toggleDropdownHandler = (self, add) => {
    const action = add ? E$1 : r;
    action(self.element, ut, dropdownClickHandler);
  };
  const getCurrentOpenDropdown = (element) => {
    const currentParent = [...dropdownMenuClasses, "btn-group", "input-group"].map((c) => No(`${c} ${showClass}`, d(element))).find((x2) => x2.length);
    if (currentParent && currentParent.length) {
      return [...currentParent[0].children].find(
        (x2) => dropdownMenuClasses.some((c) => c === Zt(x2, dataBsToggle))
      );
    }
    return void 0;
  };
  const dropdownDismissHandler = (e2) => {
    const { target, type } = e2;
    if (!target || !target.closest)
      return;
    const element = getCurrentOpenDropdown(target);
    const self = element && getDropdownInstance(element);
    if (!self)
      return;
    const { parentElement, menu } = self;
    const hasData = de(target, dropdownSelector) !== null;
    const isForm = parentElement && parentElement.contains(target) && (target.tagName === "form" || de(target, "form") !== null);
    if (type === ut && isEmptyAnchor(target)) {
      e2.preventDefault();
    }
    if (type === _ && (target === element || target === menu || menu.contains(target))) {
      return;
    }
    if (isForm || hasData)
      ;
    else if (self) {
      self.hide();
    }
  };
  const dropdownClickHandler = (e2) => {
    const { target } = e2;
    const element = target && de(target, dropdownSelector);
    const self = element && getDropdownInstance(element);
    if (self) {
      e2.stopImmediatePropagation();
      self.toggle();
      if (element && isEmptyAnchor(element))
        e2.preventDefault();
    }
  };
  const dropdownPreventScroll = (e2) => {
    if ([Qe, qe].includes(e2.code))
      e2.preventDefault();
  };
  function dropdownKeyHandler(e2) {
    const { code } = e2;
    const element = getCurrentOpenDropdown(this);
    const self = element && getDropdownInstance(element);
    const { activeElement } = element && d(element);
    if (!self || !activeElement)
      return;
    const { menu, open } = self;
    const menuItems = getMenuItems(menu);
    if (menuItems && menuItems.length && [Qe, qe].includes(code)) {
      let idx = menuItems.indexOf(activeElement);
      if (activeElement === element) {
        idx = 0;
      } else if (code === qe) {
        idx = idx > 1 ? idx - 1 : 0;
      } else if (code === Qe) {
        idx = idx < menuItems.length - 1 ? idx + 1 : idx;
      }
      if (menuItems[idx])
        Rn(menuItems[idx]);
    }
    if ($e === code && open) {
      self.toggle();
      Rn(element);
    }
  }
  function dropdownLayoutHandler() {
    const element = getCurrentOpenDropdown(this);
    const self = element && getDropdownInstance(element);
    if (self && self.open)
      styleDropdown(self);
  }
  class Dropdown extends BaseComponent {
    static selector = dropdownSelector;
    static init = dropdownInitCallback;
    static getInstance = getDropdownInstance;
    /**
     * @param target Element or string selector
     * @param config the instance options
     */
    constructor(target, config) {
      super(target, config);
      const { parentElement } = this.element;
      this.parentElement = parentElement;
      this.menu = wo(`.${dropdownMenuClass}`, parentElement);
      toggleDropdownHandler(this, true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return dropdownComponent;
    }
    /**
     * Returns component default options.
     */
    get defaults() {
      return dropdownDefaults;
    }
    // DROPDOWN PUBLIC METHODS
    // =======================
    /** Shows/hides the dropdown menu to the user. */
    toggle() {
      if (this.open)
        this.hide();
      else
        this.show();
    }
    /** Shows the dropdown menu to the user. */
    show() {
      const { element, open, menu, parentElement } = this;
      if (open)
        return;
      const currentElement = getCurrentOpenDropdown(element);
      const currentInstance = currentElement && getDropdownInstance(currentElement);
      if (currentInstance)
        currentInstance.hide();
      [showDropdownEvent, shownDropdownEvent, updatedDropdownEvent].forEach((e2) => {
        e2.relatedTarget = element;
      });
      Q(parentElement, showDropdownEvent);
      if (showDropdownEvent.defaultPrevented)
        return;
      Ln(menu, showClass);
      Ln(parentElement, showClass);
      F(element, ve, "true");
      styleDropdown(this);
      this.open = !open;
      Rn(element);
      toggleDropdownDismiss(this);
      Q(parentElement, shownDropdownEvent);
    }
    /** Hides the dropdown menu from the user. */
    hide() {
      const { element, open, menu, parentElement } = this;
      if (!open)
        return;
      [hideDropdownEvent, hiddenDropdownEvent].forEach((e2) => {
        e2.relatedTarget = element;
      });
      Q(parentElement, hideDropdownEvent);
      if (hideDropdownEvent.defaultPrevented)
        return;
      On(menu, showClass);
      On(parentElement, showClass);
      F(element, ve, "false");
      this.open = !open;
      toggleDropdownDismiss(this);
      Q(parentElement, hiddenDropdownEvent);
    }
    /** Removes the `Dropdown` component from the target element. */
    dispose() {
      if (this.open)
        this.hide();
      toggleDropdownHandler(this);
      super.dispose();
    }
  }
  const modalString = "modal";
  const modalComponent = "Modal";
  const offcanvasComponent = "Offcanvas";
  const fixedTopClass = "fixed-top";
  const fixedBottomClass = "fixed-bottom";
  const stickyTopClass = "sticky-top";
  const positionStickyClass = "position-sticky";
  const getFixedItems = (parent) => [
    ...No(fixedTopClass, parent),
    ...No(fixedBottomClass, parent),
    ...No(stickyTopClass, parent),
    ...No(positionStickyClass, parent),
    ...No("is-fixed", parent)
  ];
  const resetScrollbar = (element) => {
    const bd = _n(element);
    Yn(bd, {
      paddingRight: "",
      overflow: ""
    });
    const fixedItems = getFixedItems(bd);
    if (fixedItems.length) {
      fixedItems.forEach((fixed) => {
        Yn(fixed, {
          paddingRight: "",
          marginRight: ""
        });
      });
    }
  };
  const measureScrollbar = (element) => {
    const { clientWidth } = k(element);
    const { innerWidth } = so(element);
    return Math.abs(innerWidth - clientWidth);
  };
  const setScrollbar = (element, overflow) => {
    const bd = _n(element);
    const bodyPad = parseInt(v(bd, "paddingRight"), 10);
    const isOpen = v(bd, "overflow") === "hidden";
    const sbWidth = isOpen && bodyPad ? 0 : measureScrollbar(element);
    const fixedItems = getFixedItems(bd);
    if (overflow) {
      Yn(bd, {
        overflow: "hidden",
        paddingRight: `${bodyPad + sbWidth}px`
      });
      if (fixedItems.length) {
        fixedItems.forEach((fixed) => {
          const itemPadValue = v(fixed, "paddingRight");
          fixed.style.paddingRight = `${parseInt(itemPadValue, 10) + sbWidth}px`;
          if ([stickyTopClass, positionStickyClass].some((c) => In(fixed, c))) {
            const itemMValue = v(fixed, "marginRight");
            fixed.style.marginRight = `${parseInt(itemMValue, 10) - sbWidth}px`;
          }
        });
      }
    }
  };
  const offcanvasString = "offcanvas";
  const popupContainer = $t({ tagName: "div" });
  const appendPopup = (target, customContainer) => {
    const containerIsBody = i(customContainer) && customContainer.nodeName === "BODY";
    const lookup = i(customContainer) && !containerIsBody ? customContainer : popupContainer;
    const BODY = containerIsBody ? customContainer : _n(target);
    if (i(target)) {
      if (lookup === popupContainer) {
        BODY.append(popupContainer);
      }
      lookup.append(target);
    }
  };
  const removePopup = (target, customContainer) => {
    const containerIsBody = i(customContainer) && customContainer.nodeName === "BODY";
    const lookup = i(customContainer) && !containerIsBody ? customContainer : popupContainer;
    if (i(target)) {
      target.remove();
      if (lookup === popupContainer && !popupContainer.children.length) {
        popupContainer.remove();
      }
    }
  };
  const hasPopup = (target, customContainer) => {
    const lookup = i(customContainer) && customContainer.nodeName !== "BODY" ? customContainer : popupContainer;
    return i(target) && lookup.contains(target);
  };
  const backdropString = "backdrop";
  const modalBackdropClass = `${modalString}-${backdropString}`;
  const offcanvasBackdropClass = `${offcanvasString}-${backdropString}`;
  const modalActiveSelector = `.${modalString}.${showClass}`;
  const offcanvasActiveSelector = `.${offcanvasString}.${showClass}`;
  const overlay = $t("div");
  const getCurrentOpen = (element) => {
    return wo(`${modalActiveSelector},${offcanvasActiveSelector}`, d(element));
  };
  const toggleOverlayType = (isModal) => {
    const targetClass = isModal ? modalBackdropClass : offcanvasBackdropClass;
    [modalBackdropClass, offcanvasBackdropClass].forEach((c) => {
      On(overlay, c);
    });
    Ln(overlay, targetClass);
  };
  const appendOverlay = (element, hasFade, isModal) => {
    toggleOverlayType(isModal);
    appendPopup(overlay, _n(element));
    if (hasFade)
      Ln(overlay, fadeClass);
  };
  const showOverlay = () => {
    if (!In(overlay, showClass)) {
      Ln(overlay, showClass);
      Xn(overlay);
    }
  };
  const hideOverlay = () => {
    On(overlay, showClass);
  };
  const removeOverlay = (element) => {
    if (!getCurrentOpen(element)) {
      On(overlay, fadeClass);
      removePopup(overlay, _n(element));
      resetScrollbar(element);
    }
  };
  const isVisible = (element) => {
    return u(element) && v(element, "visibility") !== "hidden" && element.offsetParent !== null;
  };
  const modalSelector = `.${modalString}`;
  const modalToggleSelector = `[${dataBsToggle}="${modalString}"]`;
  const modalDismissSelector = `[${dataBsDismiss}="${modalString}"]`;
  const modalStaticClass = `${modalString}-static`;
  const modalDefaults = {
    backdrop: true,
    keyboard: true
  };
  const getModalInstance = (element) => Bn(element, modalComponent);
  const modalInitCallback = (element) => new Modal(element);
  const showModalEvent = Jn(`show.bs.${modalString}`);
  const shownModalEvent = Jn(`shown.bs.${modalString}`);
  const hideModalEvent = Jn(`hide.bs.${modalString}`);
  const hiddenModalEvent = Jn(`hidden.bs.${modalString}`);
  const setModalScrollbar = (self) => {
    const { element } = self;
    const scrollbarWidth = measureScrollbar(element);
    const { clientHeight, scrollHeight } = k(element);
    const { clientHeight: modalHeight, scrollHeight: modalScrollHeight } = element;
    const modalOverflow = modalHeight !== modalScrollHeight;
    if (!modalOverflow && scrollbarWidth) {
      const pad = !bo(element) ? "paddingRight" : (
        /* istanbul ignore next */
        "paddingLeft"
      );
      const padStyle = {};
      padStyle[pad] = `${scrollbarWidth}px`;
      Yn(element, padStyle);
    }
    setScrollbar(element, modalOverflow || clientHeight !== scrollHeight);
  };
  const toggleModalDismiss = (self, add) => {
    const action = add ? E$1 : r;
    const { element } = self;
    action(element, ut, modalDismissHandler);
    action(so(element), Lt, self.update, Kn);
    action(d(element), ct, modalKeyHandler);
  };
  const toggleModalHandler = (self, add) => {
    const action = add ? E$1 : r;
    const { triggers } = self;
    if (triggers.length) {
      triggers.forEach((btn) => action(btn, ut, modalClickHandler));
    }
  };
  const afterModalHide = (self, callback) => {
    const { triggers, element, relatedTarget } = self;
    removeOverlay(element);
    Yn(element, { paddingRight: "", display: "" });
    toggleModalDismiss(self);
    const focusElement = showModalEvent.relatedTarget || triggers.find(isVisible);
    if (focusElement)
      Rn(focusElement);
    if (lo(callback))
      callback();
    hiddenModalEvent.relatedTarget = relatedTarget;
    Q(element, hiddenModalEvent);
  };
  const afterModalShow = (self) => {
    const { element, relatedTarget } = self;
    Rn(element);
    toggleModalDismiss(self, true);
    shownModalEvent.relatedTarget = relatedTarget;
    Q(element, shownModalEvent);
  };
  const beforeModalShow = (self) => {
    const { element, hasFade } = self;
    Yn(element, { display: "block" });
    setModalScrollbar(self);
    if (!getCurrentOpen(element)) {
      Yn(_n(element), { overflow: "hidden" });
    }
    Ln(element, showClass);
    Dn(element, be);
    F(element, we, "true");
    if (hasFade)
      Un(element, () => afterModalShow(self));
    else
      afterModalShow(self);
  };
  const beforeModalHide = (self, callback) => {
    const { element, options, hasFade } = self;
    if (options.backdrop && !callback && hasFade && In(overlay, showClass) && !getCurrentOpen(element)) {
      hideOverlay();
      Un(overlay, () => afterModalHide(self));
    } else {
      afterModalHide(self, callback);
    }
  };
  const modalClickHandler = (e2) => {
    const { target } = e2;
    const trigger = target && de(target, modalToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getModalInstance(element);
    if (!self)
      return;
    if (trigger && trigger.tagName === "A")
      e2.preventDefault();
    self.relatedTarget = trigger;
    self.toggle();
  };
  const modalKeyHandler = ({ code, target }) => {
    const element = wo(modalActiveSelector, d(target));
    const self = element && getModalInstance(element);
    if (!self)
      return;
    const { options } = self;
    if (options.keyboard && code === $e && // the keyboard option is enabled and the key is 27
    In(element, showClass)) {
      self.relatedTarget = null;
      self.hide();
    }
  };
  function modalDismissHandler(e2) {
    const self = getModalInstance(this);
    if (!self || Zn.get(this))
      return;
    const { options, isStatic, modalDialog } = self;
    const { backdrop } = options;
    const { target } = e2;
    const selectedText = d(this)?.getSelection()?.toString().length;
    const targetInsideDialog = modalDialog?.contains(target);
    const dismiss = target && de(target, modalDismissSelector);
    if (isStatic && !targetInsideDialog) {
      Zn.set(
        this,
        () => {
          Ln(this, modalStaticClass);
          Un(modalDialog, () => staticTransitionEnd(self));
        },
        17
      );
    } else if (dismiss || !selectedText && !isStatic && !targetInsideDialog && backdrop) {
      self.relatedTarget = dismiss || null;
      self.hide();
      e2.preventDefault();
    }
  }
  const staticTransitionEnd = (self) => {
    const { element, modalDialog } = self;
    const duration = (u(modalDialog) ? oe(modalDialog) : (
      /* istanbul ignore next */
      0
    )) + 17;
    On(element, modalStaticClass);
    Zn.set(element, () => Zn.clear(element), duration);
  };
  class Modal extends BaseComponent {
    static selector = modalSelector;
    static init = modalInitCallback;
    static getInstance = getModalInstance;
    /**
     * @param target usually the `.modal` element
     * @param config instance options
     */
    constructor(target, config) {
      super(target, config);
      const { element } = this;
      this.modalDialog = wo(`.${modalString}-dialog`, element);
      this.triggers = [...Mo(modalToggleSelector, d(element))].filter(
        (btn) => getTargetElement(btn) === element
      );
      this.isStatic = this.options.backdrop === "static";
      this.hasFade = In(element, fadeClass);
      this.relatedTarget = null;
      toggleModalHandler(this, true);
      this.update = this.update.bind(this);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return modalComponent;
    }
    /**
     * Returns component default options.
     */
    get defaults() {
      return modalDefaults;
    }
    // MODAL PUBLIC METHODS
    // ====================
    /** Toggles the visibility of the modal. */
    toggle() {
      if (In(this.element, showClass))
        this.hide();
      else
        this.show();
    }
    /** Shows the modal to the user. */
    show() {
      const { element, options, hasFade, relatedTarget } = this;
      const { backdrop } = options;
      let overlayDelay = 0;
      if (In(element, showClass))
        return;
      showModalEvent.relatedTarget = relatedTarget || void 0;
      Q(element, showModalEvent);
      if (showModalEvent.defaultPrevented)
        return;
      const currentOpen = getCurrentOpen(element);
      if (currentOpen && currentOpen !== element) {
        const that = getModalInstance(currentOpen) || /* istanbul ignore next */
        Bn(currentOpen, offcanvasComponent);
        if (that)
          that.hide();
      }
      if (backdrop) {
        if (!hasPopup(overlay)) {
          appendOverlay(element, hasFade, true);
        } else {
          toggleOverlayType(true);
        }
        overlayDelay = oe(overlay);
        showOverlay();
        setTimeout(() => beforeModalShow(this), overlayDelay);
      } else {
        beforeModalShow(this);
        if (currentOpen && In(overlay, showClass)) {
          hideOverlay();
        }
      }
    }
    /**
     * Hide the modal from the user.
     *
     * @param callback when defined it will skip animation
     */
    hide(callback) {
      const { element, hasFade, relatedTarget } = this;
      if (!In(element, showClass))
        return;
      hideModalEvent.relatedTarget = relatedTarget || void 0;
      Q(element, hideModalEvent);
      if (hideModalEvent.defaultPrevented)
        return;
      On(element, showClass);
      F(element, be, "true");
      Dn(element, we);
      if (hasFade) {
        Un(element, () => beforeModalHide(this, callback));
      } else {
        beforeModalHide(this, callback);
      }
    }
    /**
     * Updates the modal layout.
     */
    update() {
      if (In(this.element, showClass))
        setModalScrollbar(this);
    }
    /** Removes the `Modal` component from target element. */
    dispose() {
      toggleModalHandler(this);
      this.hide(() => super.dispose());
    }
  }
  const offcanvasSelector = `.${offcanvasString}`;
  const offcanvasToggleSelector = `[${dataBsToggle}="${offcanvasString}"]`;
  const offcanvasDismissSelector = `[${dataBsDismiss}="${offcanvasString}"]`;
  const offcanvasTogglingClass = `${offcanvasString}-toggling`;
  const offcanvasDefaults = {
    backdrop: true,
    // boolean
    keyboard: true,
    // boolean
    scroll: false
    // boolean
  };
  const getOffcanvasInstance = (element) => Bn(element, offcanvasComponent);
  const offcanvasInitCallback = (element) => new Offcanvas(element);
  const showOffcanvasEvent = Jn(`show.bs.${offcanvasString}`);
  const shownOffcanvasEvent = Jn(`shown.bs.${offcanvasString}`);
  const hideOffcanvasEvent = Jn(`hide.bs.${offcanvasString}`);
  const hiddenOffcanvasEvent = Jn(`hidden.bs.${offcanvasString}`);
  const setOffCanvasScrollbar = (self) => {
    const { element } = self;
    const { clientHeight, scrollHeight } = k(element);
    setScrollbar(element, clientHeight !== scrollHeight);
  };
  const toggleOffcanvasEvents = (self, add) => {
    const action = add ? E$1 : r;
    self.triggers.forEach((btn) => action(btn, ut, offcanvasTriggerHandler));
  };
  const toggleOffCanvasDismiss = (self, add) => {
    const action = add ? E$1 : r;
    const doc = d(self.element);
    action(doc, ct, offcanvasKeyDismissHandler);
    action(doc, ut, offcanvasDismissHandler);
  };
  const beforeOffcanvasShow = (self) => {
    const { element, options } = self;
    if (!options.scroll) {
      setOffCanvasScrollbar(self);
      Yn(_n(element), { overflow: "hidden" });
    }
    Ln(element, offcanvasTogglingClass);
    Ln(element, showClass);
    Yn(element, { visibility: "visible" });
    Un(element, () => showOffcanvasComplete(self));
  };
  const beforeOffcanvasHide = (self, callback) => {
    const { element, options } = self;
    const currentOpen = getCurrentOpen(element);
    element.blur();
    if (!currentOpen && options.backdrop && In(overlay, showClass)) {
      hideOverlay();
      Un(overlay, () => hideOffcanvasComplete(self, callback));
    } else
      hideOffcanvasComplete(self, callback);
  };
  const offcanvasTriggerHandler = (e2) => {
    const trigger = de(e2.target, offcanvasToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getOffcanvasInstance(element);
    if (self) {
      self.relatedTarget = trigger;
      self.toggle();
      if (trigger && trigger.tagName === "A") {
        e2.preventDefault();
      }
    }
  };
  const offcanvasDismissHandler = (e2) => {
    const { target } = e2;
    const element = wo(offcanvasActiveSelector, d(target));
    const offCanvasDismiss = wo(offcanvasDismissSelector, element);
    const self = element && getOffcanvasInstance(element);
    if (!self)
      return;
    const { options, triggers } = self;
    const { backdrop } = options;
    const trigger = de(target, offcanvasToggleSelector);
    const selection = d(element).getSelection();
    if (overlay.contains(target) && backdrop === "static")
      return;
    if (!(selection && selection.toString().length) && (!element.contains(target) && backdrop && /* istanbul ignore next */
    (!trigger || triggers.includes(target)) || offCanvasDismiss && offCanvasDismiss.contains(target))) {
      self.relatedTarget = offCanvasDismiss && offCanvasDismiss.contains(target) ? offCanvasDismiss : null;
      self.hide();
    }
    if (trigger && trigger.tagName === "A")
      e2.preventDefault();
  };
  const offcanvasKeyDismissHandler = ({ code, target }) => {
    const element = wo(offcanvasActiveSelector, d(target));
    const self = element && getOffcanvasInstance(element);
    if (!self)
      return;
    if (self.options.keyboard && code === $e) {
      self.relatedTarget = null;
      self.hide();
    }
  };
  const showOffcanvasComplete = (self) => {
    const { element } = self;
    On(element, offcanvasTogglingClass);
    Dn(element, be);
    F(element, we, "true");
    F(element, "role", "dialog");
    Q(element, shownOffcanvasEvent);
    toggleOffCanvasDismiss(self, true);
    Rn(element);
  };
  const hideOffcanvasComplete = (self, callback) => {
    const { element, triggers } = self;
    F(element, be, "true");
    Dn(element, we);
    Dn(element, "role");
    Yn(element, { visibility: "" });
    const visibleTrigger = showOffcanvasEvent.relatedTarget || triggers.find(isVisible);
    if (visibleTrigger)
      Rn(visibleTrigger);
    removeOverlay(element);
    Q(element, hiddenOffcanvasEvent);
    On(element, offcanvasTogglingClass);
    if (!getCurrentOpen(element)) {
      toggleOffCanvasDismiss(self);
    }
    if (lo(callback))
      callback();
  };
  class Offcanvas extends BaseComponent {
    static selector = offcanvasSelector;
    static init = offcanvasInitCallback;
    static getInstance = getOffcanvasInstance;
    /**
     * @param target usually an `.offcanvas` element
     * @param config instance options
     */
    constructor(target, config) {
      super(target, config);
      const { element } = this;
      this.triggers = [...Mo(offcanvasToggleSelector, d(element))].filter(
        (btn) => getTargetElement(btn) === element
      );
      this.relatedTarget = null;
      toggleOffcanvasEvents(this, true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return offcanvasComponent;
    }
    /**
     * Returns component default options.
     */
    get defaults() {
      return offcanvasDefaults;
    }
    // OFFCANVAS PUBLIC METHODS
    // ========================
    /** Shows or hides the offcanvas from the user. */
    toggle() {
      if (In(this.element, showClass))
        this.hide();
      else
        this.show();
    }
    /** Shows the offcanvas to the user. */
    show() {
      const { element, options, relatedTarget } = this;
      let overlayDelay = 0;
      if (In(element, showClass))
        return;
      showOffcanvasEvent.relatedTarget = relatedTarget || void 0;
      shownOffcanvasEvent.relatedTarget = relatedTarget || void 0;
      Q(element, showOffcanvasEvent);
      if (showOffcanvasEvent.defaultPrevented)
        return;
      const currentOpen = getCurrentOpen(element);
      if (currentOpen && currentOpen !== element) {
        const that = getOffcanvasInstance(currentOpen) || /* istanbul ignore next */
        Bn(currentOpen, modalComponent);
        if (that)
          that.hide();
      }
      if (options.backdrop) {
        if (!hasPopup(overlay)) {
          appendOverlay(element, true);
        } else {
          toggleOverlayType();
        }
        overlayDelay = oe(overlay);
        showOverlay();
        setTimeout(() => beforeOffcanvasShow(this), overlayDelay);
      } else {
        beforeOffcanvasShow(this);
        if (currentOpen && In(overlay, showClass)) {
          hideOverlay();
        }
      }
    }
    /**
     * Hides the offcanvas from the user.
     *
     * @param callback when `true` it will skip animation
     */
    hide(callback) {
      const { element, relatedTarget } = this;
      if (!In(element, showClass))
        return;
      hideOffcanvasEvent.relatedTarget = relatedTarget || void 0;
      hiddenOffcanvasEvent.relatedTarget = relatedTarget || void 0;
      Q(element, hideOffcanvasEvent);
      if (hideOffcanvasEvent.defaultPrevented)
        return;
      Ln(element, offcanvasTogglingClass);
      On(element, showClass);
      if (!callback) {
        Un(element, () => beforeOffcanvasHide(this, callback));
      } else
        beforeOffcanvasHide(this, callback);
    }
    /** Removes the `Offcanvas` from the target element. */
    dispose() {
      toggleOffcanvasEvents(this);
      this.hide(() => super.dispose());
    }
  }
  const popoverString = "popover";
  const popoverComponent = "Popover";
  const tooltipString = "tooltip";
  const getTipTemplate = (tipType) => {
    const isTooltip = tipType === tooltipString;
    const bodyClass = isTooltip ? `${tipType}-inner` : `${tipType}-body`;
    const header = !isTooltip ? `<h3 class="${tipType}-header"></h3>` : "";
    const arrow = `<div class="${tipType}-arrow"></div>`;
    const body = `<div class="${bodyClass}"></div>`;
    return `<div class="${tipType}" role="${tooltipString}">${header + arrow + body}</div>`;
  };
  const tipClassPositions = {
    top: "top",
    bottom: "bottom",
    left: "start",
    right: "end"
  };
  const styleTip = (self) => {
    const tipClasses = /\b(top|bottom|start|end)+/;
    const { element, tooltip, container, options, arrow } = self;
    if (!tooltip)
      return;
    const tipPositions = { ...tipClassPositions };
    const RTL = bo(element);
    Yn(tooltip, {
      // top: '0px', left: '0px', right: '', bottom: '',
      top: "",
      left: "",
      right: "",
      bottom: ""
    });
    const isPopover = self.name === popoverComponent;
    const { offsetWidth: tipWidth, offsetHeight: tipHeight } = tooltip;
    const { clientWidth: htmlcw, clientHeight: htmlch, offsetWidth: htmlow } = k(element);
    let { placement } = options;
    const { clientWidth: parentCWidth, offsetWidth: parentOWidth } = container;
    const parentPosition = v(container, "position");
    const fixedParent = parentPosition === "fixed";
    const scrollbarWidth = fixedParent ? Math.abs(parentCWidth - parentOWidth) : Math.abs(htmlcw - htmlow);
    const leftBoundry = RTL && fixedParent ? (
      /* istanbul ignore next */
      scrollbarWidth
    ) : 0;
    const rightBoundry = htmlcw - (!RTL ? scrollbarWidth : 0) - 1;
    const {
      width: elemWidth,
      height: elemHeight,
      left: elemRectLeft,
      right: elemRectRight,
      top: elemRectTop
    } = w(element, true);
    const { x: x2, y: y2 } = {
      x: elemRectLeft,
      y: elemRectTop
    };
    Yn(arrow, {
      top: "",
      left: "",
      right: "",
      bottom: ""
    });
    let topPosition = 0;
    let bottomPosition = "";
    let leftPosition = 0;
    let rightPosition = "";
    let arrowTop = "";
    let arrowLeft = "";
    let arrowRight = "";
    const arrowWidth = arrow.offsetWidth || 0;
    const arrowHeight = arrow.offsetHeight || 0;
    const arrowAdjust = arrowWidth / 2;
    let topExceed = elemRectTop - tipHeight - arrowHeight < 0;
    let bottomExceed = elemRectTop + tipHeight + elemHeight + arrowHeight >= htmlch;
    let leftExceed = elemRectLeft - tipWidth - arrowWidth < leftBoundry;
    let rightExceed = elemRectLeft + tipWidth + elemWidth + arrowWidth >= rightBoundry;
    const horizontals = ["left", "right"];
    const verticals = ["top", "bottom"];
    topExceed = horizontals.includes(placement) ? elemRectTop + elemHeight / 2 - tipHeight / 2 - arrowHeight < 0 : topExceed;
    bottomExceed = horizontals.includes(placement) ? elemRectTop + tipHeight / 2 + elemHeight / 2 + arrowHeight >= htmlch : bottomExceed;
    leftExceed = verticals.includes(placement) ? elemRectLeft + elemWidth / 2 - tipWidth / 2 < leftBoundry : leftExceed;
    rightExceed = verticals.includes(placement) ? elemRectLeft + tipWidth / 2 + elemWidth / 2 >= rightBoundry : rightExceed;
    placement = horizontals.includes(placement) && leftExceed && rightExceed ? "top" : placement;
    placement = placement === "top" && topExceed ? "bottom" : placement;
    placement = placement === "bottom" && bottomExceed ? "top" : placement;
    placement = placement === "left" && leftExceed ? "right" : placement;
    placement = placement === "right" && rightExceed ? "left" : placement;
    if (!tooltip.className.includes(placement)) {
      tooltip.className = tooltip.className.replace(tipClasses, tipPositions[placement]);
    }
    if (horizontals.includes(placement)) {
      if (placement === "left") {
        leftPosition = x2 - tipWidth - (isPopover ? arrowWidth : 0);
      } else {
        leftPosition = x2 + elemWidth + (isPopover ? arrowWidth : 0);
      }
      if (topExceed && bottomExceed) {
        topPosition = 0;
        bottomPosition = 0;
        arrowTop = elemRectTop + elemHeight / 2 - arrowHeight / 2;
      } else if (topExceed) {
        topPosition = y2;
        bottomPosition = "";
        arrowTop = elemHeight / 2 - arrowWidth;
      } else if (bottomExceed) {
        topPosition = y2 - tipHeight + elemHeight;
        bottomPosition = "";
        arrowTop = tipHeight - elemHeight / 2 - arrowWidth;
      } else {
        topPosition = y2 - tipHeight / 2 + elemHeight / 2;
        arrowTop = tipHeight / 2 - arrowHeight / 2;
      }
    } else if (verticals.includes(placement)) {
      if (placement === "top") {
        topPosition = y2 - tipHeight - (isPopover ? arrowHeight : 0);
      } else {
        topPosition = y2 + elemHeight + (isPopover ? arrowHeight : 0);
      }
      if (leftExceed) {
        leftPosition = 0;
        arrowLeft = x2 + elemWidth / 2 - arrowAdjust;
      } else if (rightExceed) {
        leftPosition = "auto";
        rightPosition = 0;
        arrowRight = elemWidth / 2 + rightBoundry - elemRectRight - arrowAdjust;
      } else {
        leftPosition = x2 - tipWidth / 2 + elemWidth / 2;
        arrowLeft = tipWidth / 2 - arrowAdjust;
      }
    }
    Yn(tooltip, {
      top: `${topPosition}px`,
      bottom: bottomPosition === "" ? "" : `${bottomPosition}px`,
      left: leftPosition === "auto" ? leftPosition : `${leftPosition}px`,
      right: rightPosition !== "" ? `${rightPosition}px` : ""
    });
    if (u(arrow)) {
      if (arrowTop !== "") {
        arrow.style.top = `${arrowTop}px`;
      }
      if (arrowLeft !== "") {
        arrow.style.left = `${arrowLeft}px`;
      } else if (arrowRight !== "") {
        arrow.style.right = `${arrowRight}px`;
      }
    }
    const updatedTooltipEvent = Jn(`updated.bs.${se(self.name)}`);
    Q(element, updatedTooltipEvent);
  };
  const tooltipDefaults = {
    template: getTipTemplate(tooltipString),
    title: "",
    customClass: "",
    trigger: "hover focus",
    placement: "top",
    sanitizeFn: void 0,
    animation: true,
    delay: 200,
    container: document.body,
    content: "",
    dismissible: false,
    btnClose: ""
  };
  const dataOriginalTitle = "data-original-title";
  const tooltipComponent = "Tooltip";
  const setHtml = (element, content, sanitizeFn) => {
    if (!u(element) || g(content) && !content.length)
      return;
    if (g(content)) {
      let dirty = content.trim();
      if (lo(sanitizeFn))
        dirty = sanitizeFn(dirty);
      const domParser = new DOMParser();
      const tempDocument = domParser.parseFromString(dirty, "text/html");
      element.append(...[...tempDocument.body.childNodes]);
    } else if (u(content)) {
      element.append(content);
    } else if (Eo(content) || ue(content) && content.every(i)) {
      element.append(...[...content]);
    }
  };
  const createTip = (self) => {
    const isTooltip = self.name === tooltipComponent;
    const { id, element, options } = self;
    const { title, placement, template, animation, customClass, sanitizeFn, dismissible, content, btnClose } = options;
    const tipString = isTooltip ? tooltipString : popoverString;
    const tipPositions = { ...tipClassPositions };
    let titleParts = [];
    let contentParts = [];
    if (bo(element)) {
      tipPositions.left = "end";
      tipPositions.right = "start";
    }
    const placementClass = `bs-${tipString}-${tipPositions[placement]}`;
    let tooltipTemplate;
    if (u(template)) {
      tooltipTemplate = template;
    } else {
      const htmlMarkup = $t("div");
      setHtml(htmlMarkup, template, sanitizeFn);
      tooltipTemplate = htmlMarkup.firstChild;
    }
    self.tooltip = u(tooltipTemplate) ? tooltipTemplate.cloneNode(true) : (
      /* istanbul ignore next */
      void 0
    );
    const { tooltip } = self;
    if (!tooltip)
      return;
    F(tooltip, "id", id);
    F(tooltip, "role", tooltipString);
    const bodyClass = isTooltip ? `${tooltipString}-inner` : `${popoverString}-body`;
    const tooltipHeader = isTooltip ? null : wo(`.${popoverString}-header`, tooltip);
    const tooltipBody = wo(`.${bodyClass}`, tooltip);
    self.arrow = wo(`.${tipString}-arrow`, tooltip);
    const { arrow } = self;
    if (u(title))
      titleParts = [title.cloneNode(true)];
    else {
      const tempTitle = $t("div");
      setHtml(tempTitle, title, sanitizeFn);
      titleParts = [...[...tempTitle.childNodes]];
    }
    if (u(content))
      contentParts = [content.cloneNode(true)];
    else {
      const tempContent = $t("div");
      setHtml(tempContent, content, sanitizeFn);
      contentParts = [...[...tempContent.childNodes]];
    }
    if (dismissible) {
      if (title) {
        if (u(btnClose))
          titleParts = [...titleParts, btnClose.cloneNode(true)];
        else {
          const tempBtn = $t("div");
          setHtml(tempBtn, btnClose, sanitizeFn);
          titleParts = [...titleParts, tempBtn.firstChild];
        }
      } else {
        if (tooltipHeader)
          tooltipHeader.remove();
        if (u(btnClose))
          contentParts = [...contentParts, btnClose.cloneNode(true)];
        else {
          const tempBtn = $t("div");
          setHtml(tempBtn, btnClose, sanitizeFn);
          contentParts = [...contentParts, tempBtn.firstChild];
        }
      }
    }
    if (!isTooltip) {
      if (title && tooltipHeader)
        setHtml(tooltipHeader, titleParts, sanitizeFn);
      if (content && tooltipBody)
        setHtml(tooltipBody, contentParts, sanitizeFn);
      self.btn = wo(".btn-close", tooltip) || void 0;
    } else if (title && tooltipBody)
      setHtml(tooltipBody, title, sanitizeFn);
    Ln(tooltip, "position-fixed");
    Ln(arrow, "position-absolute");
    if (!In(tooltip, tipString))
      Ln(tooltip, tipString);
    if (animation && !In(tooltip, fadeClass))
      Ln(tooltip, fadeClass);
    if (customClass && !In(tooltip, customClass)) {
      Ln(tooltip, customClass);
    }
    if (!In(tooltip, placementClass))
      Ln(tooltip, placementClass);
  };
  const getElementContainer = (element) => {
    const majorBlockTags = ["HTML", "BODY"];
    const containers = [];
    let { parentNode } = element;
    while (parentNode && !majorBlockTags.includes(parentNode.nodeName)) {
      parentNode = no(parentNode);
      if (!(re(parentNode) || yo(parentNode))) {
        containers.push(parentNode);
      }
    }
    return containers.find((c, i2) => {
      if (v(c, "position") !== "relative" && containers.slice(i2 + 1).every((r2) => v(r2, "position") === "static")) {
        return c;
      }
      return null;
    }) || /* istanbul ignore next: optional guard */
    d(element).body;
  };
  const tooltipSelector = `[${dataBsToggle}="${tooltipString}"],[data-tip="${tooltipString}"]`;
  const titleAttr = "title";
  let getTooltipInstance = (element) => Bn(element, tooltipComponent);
  const tooltipInitCallback = (element) => new Tooltip(element);
  const removeTooltip = (self) => {
    const { element, tooltip, container, offsetParent } = self;
    Dn(element, ge);
    removePopup(tooltip, container === offsetParent ? container : offsetParent);
  };
  const disposeTooltipComplete = (self, callback) => {
    const { element } = self;
    toggleTooltipHandlers(self);
    if (Nn(element, dataOriginalTitle) && self.name === tooltipComponent) {
      toggleTooltipTitle(self);
    }
    if (callback)
      callback();
  };
  const toggleTooltipAction = (self, add) => {
    const action = add ? E$1 : r;
    const { element } = self;
    action(d(element), Bt, self.handleTouch, Kn);
    [xt, Lt].forEach((ev) => {
      action(so(element), ev, self.update, Kn);
    });
  };
  const tooltipShownAction = (self) => {
    const { element } = self;
    const shownTooltipEvent = Jn(`shown.bs.${se(self.name)}`);
    toggleTooltipAction(self, true);
    Q(element, shownTooltipEvent);
    Zn.clear(element, "in");
  };
  const tooltipHiddenAction = (self) => {
    const { element, onHideComplete } = self;
    const hiddenTooltipEvent = Jn(`hidden.bs.${se(self.name)}`);
    toggleTooltipAction(self);
    removeTooltip(self);
    Q(element, hiddenTooltipEvent);
    if (lo(onHideComplete)) {
      onHideComplete();
      self.onHideComplete = void 0;
    }
    Zn.clear(element, "out");
  };
  const toggleTooltipHandlers = (self, add) => {
    const action = add ? E$1 : r;
    const { element, options, btn } = self;
    const { trigger } = options;
    const isPopover = self.name !== tooltipComponent;
    const dismissible = isPopover && options.dismissible ? true : false;
    if (trigger.includes("manual"))
      return;
    self.enabled = !!add;
    const triggerOptions = trigger.split(" ");
    triggerOptions.forEach((tr) => {
      if (tr === ft) {
        action(element, dt, self.show);
        action(element, mt, self.show);
        if (dismissible && btn) {
          action(btn, ut, self.hide);
        } else {
          action(element, gt, self.hide);
          action(d(element), Bt, self.handleTouch, Kn);
        }
      } else if (tr === ut) {
        action(element, tr, !dismissible ? self.toggle : self.show);
      } else if (tr === _) {
        action(element, tt, self.show);
        if (!dismissible)
          action(element, et, self.hide);
        if (vn) {
          action(element, ut, () => Rn(element));
        }
      }
    });
  };
  const toggleTooltipOpenHandlers = (self, add) => {
    const action = add ? E$1 : r;
    const { element, container, offsetParent } = self;
    const { offsetHeight, scrollHeight } = container;
    const parentModal = de(element, `.${modalString}`);
    const parentOffcanvas = de(element, `.${offcanvasString}`);
    const win = so(element);
    const overflow = offsetHeight !== scrollHeight;
    const scrollTarget = container === offsetParent && overflow ? container : win;
    action(scrollTarget, Lt, self.update, Kn);
    action(scrollTarget, xt, self.update, Kn);
    if (parentModal)
      action(parentModal, `hide.bs.${modalString}`, self.hide);
    if (parentOffcanvas)
      action(parentOffcanvas, `hide.bs.${offcanvasString}`, self.hide);
  };
  const toggleTooltipTitle = (self, content) => {
    const titleAtt = [dataOriginalTitle, titleAttr];
    const { element } = self;
    F(
      element,
      titleAtt[content ? 0 : 1],
      content || Zt(element, titleAtt[0]) || /* istanbul ignore next */
      ""
    );
    Dn(element, titleAtt[content ? 1 : 0]);
  };
  class Tooltip extends BaseComponent {
    static selector = tooltipSelector;
    static init = tooltipInitCallback;
    static getInstance = getTooltipInstance;
    static styleTip = styleTip;
    /**
     * @param target the target element
     * @param config the instance options
     */
    constructor(target, config) {
      super(target, config);
      const { element } = this;
      const isTooltip = this.name === tooltipComponent;
      const tipString = isTooltip ? tooltipString : popoverString;
      const tipComponent = isTooltip ? tooltipComponent : popoverComponent;
      getTooltipInstance = (elem) => Bn(elem, tipComponent);
      this.enabled = true;
      this.id = `${tipString}-${ie(element, tipString)}`;
      const { options } = this;
      if (!options.title && isTooltip || !isTooltip && !options.content) {
        return;
      }
      q(tooltipDefaults, { titleAttr: "" });
      this.handleTouch = this.handleTouch.bind(this);
      this.update = this.update.bind(this);
      this.show = this.show.bind(this);
      this.hide = this.hide.bind(this);
      this.toggle = this.toggle.bind(this);
      if (Nn(element, titleAttr) && isTooltip && typeof options.title === "string") {
        toggleTooltipTitle(this, options.title);
      }
      this.container = getElementContainer(element);
      this.offsetParent = ["sticky", "fixed"].some(
        (position) => v(this.container, "position") === position
      ) ? this.container : d(this.element).body;
      createTip(this);
      toggleTooltipHandlers(this, true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return tooltipComponent;
    }
    /**
     * Returns component default options.
     */
    get defaults() {
      return tooltipDefaults;
    }
    // TOOLTIP PUBLIC METHODS
    // ======================
    /** Shows the tooltip. */
    show() {
      const { options, tooltip, element, container, offsetParent, id } = this;
      const { animation } = options;
      const outTimer = Zn.get(element, "out");
      const tipContainer = container === offsetParent ? container : offsetParent;
      Zn.clear(element, "out");
      if (tooltip && !outTimer && !hasPopup(tooltip, tipContainer)) {
        Zn.set(
          element,
          () => {
            const showTooltipEvent = Jn(`show.bs.${se(this.name)}`);
            Q(element, showTooltipEvent);
            if (showTooltipEvent.defaultPrevented)
              return;
            appendPopup(tooltip, tipContainer);
            F(element, ge, `#${id}`);
            this.update();
            toggleTooltipOpenHandlers(this, true);
            if (!In(tooltip, showClass))
              Ln(tooltip, showClass);
            if (animation)
              Un(tooltip, () => tooltipShownAction(this));
            else
              tooltipShownAction(this);
          },
          17,
          "in"
        );
      }
    }
    /** Hides the tooltip. */
    hide() {
      const { options, tooltip, element, container, offsetParent } = this;
      const { animation, delay } = options;
      Zn.clear(element, "in");
      if (tooltip && hasPopup(tooltip, container === offsetParent ? container : offsetParent)) {
        Zn.set(
          element,
          () => {
            const hideTooltipEvent = Jn(`hide.bs.${se(this.name)}`);
            Q(element, hideTooltipEvent);
            if (hideTooltipEvent.defaultPrevented)
              return;
            this.update();
            On(tooltip, showClass);
            toggleTooltipOpenHandlers(this);
            if (animation)
              Un(tooltip, () => tooltipHiddenAction(this));
            else
              tooltipHiddenAction(this);
          },
          delay + 17,
          "out"
        );
      }
    }
    /** Updates the tooltip position. */
    update() {
      styleTip(this);
    }
    /** Toggles the tooltip visibility. */
    toggle() {
      const { tooltip, container, offsetParent } = this;
      if (tooltip && !hasPopup(tooltip, container === offsetParent ? container : offsetParent))
        this.show();
      else
        this.hide();
    }
    /** Enables the tooltip. */
    enable() {
      const { enabled } = this;
      if (!enabled) {
        toggleTooltipHandlers(this, true);
        this.enabled = !enabled;
      }
    }
    /** Disables the tooltip. */
    disable() {
      const { tooltip, container, offsetParent, options, enabled } = this;
      const { animation } = options;
      if (enabled) {
        if (tooltip && hasPopup(tooltip, container === offsetParent ? container : offsetParent) && animation) {
          this.onHideComplete = () => toggleTooltipHandlers(this);
          this.hide();
        } else {
          toggleTooltipHandlers(this);
        }
        this.enabled = !enabled;
      }
    }
    /** Toggles the `disabled` property. */
    toggleEnabled() {
      if (!this.enabled)
        this.enable();
      else
        this.disable();
    }
    /**
     * Handles the `touchstart` event listener for `Tooltip`
     *
     * @this {Tooltip}
     * @param {TouchEvent} e the `Event` object
     */
    handleTouch({ target }) {
      const { tooltip, element } = this;
      if (tooltip && tooltip.contains(target) || target === element || target && element.contains(target))
        ;
      else {
        this.hide();
      }
    }
    /** Removes the `Tooltip` from the target element. */
    dispose() {
      const { tooltip, container, offsetParent, options } = this;
      const callback = () => disposeTooltipComplete(this, () => super.dispose());
      if (options.animation && tooltip && hasPopup(tooltip, container === offsetParent ? container : offsetParent)) {
        this.options.delay = 0;
        this.onHideComplete = callback;
        this.hide();
      } else {
        callback();
      }
    }
  }
  const popoverSelector = `[${dataBsToggle}="${popoverString}"],[data-tip="${popoverString}"]`;
  const popoverDefaults = q({}, tooltipDefaults, {
    template: getTipTemplate(popoverString),
    content: "",
    dismissible: false,
    btnClose: '<button class="btn-close" aria-label="Close"></button>'
  });
  const getPopoverInstance = (element) => Bn(element, popoverComponent);
  const popoverInitCallback = (element) => new Popover(element);
  class Popover extends Tooltip {
    static selector = popoverSelector;
    static init = popoverInitCallback;
    static getInstance = getPopoverInstance;
    static styleTip = styleTip;
    /**
     * @param target the target element
     * @param config the instance options
     */
    constructor(target, config) {
      super(target, config);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return popoverComponent;
    }
    /**
     * Returns component default options.
     */
    get defaults() {
      return popoverDefaults;
    }
    /* extend original `show()` */
    show() {
      super.show();
      const { options, btn } = this;
      if (options.dismissible && btn)
        setTimeout(() => Rn(btn), 17);
    }
  }
  const tabString = "tab";
  const tabComponent = "Tab";
  const tabSelector = `[${dataBsToggle}="${tabString}"]`;
  const getTabInstance = (element) => Bn(element, tabComponent);
  const tabInitCallback = (element) => new Tab(element);
  const showTabEvent = Jn(`show.bs.${tabString}`);
  const shownTabEvent = Jn(`shown.bs.${tabString}`);
  const hideTabEvent = Jn(`hide.bs.${tabString}`);
  const hiddenTabEvent = Jn(`hidden.bs.${tabString}`);
  const tabPrivate = /* @__PURE__ */ new Map();
  const triggerTabEnd = (self) => {
    const { tabContent, nav } = self;
    if (tabContent && In(tabContent, collapsingClass)) {
      tabContent.style.height = "";
      On(tabContent, collapsingClass);
    }
    if (nav)
      Zn.clear(nav);
  };
  const triggerTabShow = (self) => {
    const { element, tabContent, content: nextContent, nav } = self;
    const { tab } = u(nav) && tabPrivate.get(nav) || /* istanbul ignore next */
    { tab: null };
    if (tabContent && nextContent && In(nextContent, fadeClass)) {
      const { currentHeight, nextHeight } = tabPrivate.get(element) || /* istanbul ignore next */
      {
        currentHeight: 0,
        nextHeight: 0
      };
      if (currentHeight === nextHeight) {
        triggerTabEnd(self);
      } else {
        setTimeout(() => {
          tabContent.style.height = `${nextHeight}px`;
          Xn(tabContent);
          Un(tabContent, () => triggerTabEnd(self));
        }, 50);
      }
    } else if (nav)
      Zn.clear(nav);
    shownTabEvent.relatedTarget = tab;
    Q(element, shownTabEvent);
  };
  const triggerTabHide = (self) => {
    const { element, content: nextContent, tabContent, nav } = self;
    const { tab, content } = nav && tabPrivate.get(nav) || /* istanbul ignore next */
    { tab: null, content: null };
    let currentHeight = 0;
    if (tabContent && nextContent && In(nextContent, fadeClass)) {
      [content, nextContent].forEach((c) => {
        if (u(c))
          Ln(c, "overflow-hidden");
      });
      currentHeight = u(content) ? content.scrollHeight : (
        /* istanbul ignore next */
        0
      );
    }
    showTabEvent.relatedTarget = tab;
    hiddenTabEvent.relatedTarget = element;
    Q(element, showTabEvent);
    if (showTabEvent.defaultPrevented)
      return;
    if (nextContent)
      Ln(nextContent, activeClass);
    if (content)
      On(content, activeClass);
    if (tabContent && nextContent && In(nextContent, fadeClass)) {
      const nextHeight = nextContent.scrollHeight;
      tabPrivate.set(element, { currentHeight, nextHeight, tab: null, content: null });
      Ln(tabContent, collapsingClass);
      tabContent.style.height = `${currentHeight}px`;
      Xn(tabContent);
      [content, nextContent].forEach((c) => {
        if (c)
          On(c, "overflow-hidden");
      });
    }
    if (nextContent && nextContent && In(nextContent, fadeClass)) {
      setTimeout(() => {
        Ln(nextContent, showClass);
        Un(nextContent, () => {
          triggerTabShow(self);
        });
      }, 1);
    } else {
      if (nextContent)
        Ln(nextContent, showClass);
      triggerTabShow(self);
    }
    if (tab)
      Q(tab, hiddenTabEvent);
  };
  const getActiveTab = (self) => {
    const { nav } = self;
    if (!u(nav))
      return { tab: null, content: null };
    const activeTabs = No(activeClass, nav);
    let tab = null;
    if (activeTabs.length === 1 && !dropdownMenuClasses.some((c) => In(activeTabs[0].parentElement, c))) {
      [tab] = activeTabs;
    } else if (activeTabs.length > 1) {
      tab = activeTabs[activeTabs.length - 1];
    }
    const content = u(tab) ? getTargetElement(tab) : null;
    return { tab, content };
  };
  const getParentDropdown = (element) => {
    if (!u(element))
      return null;
    const dropdown = de(element, `.${dropdownMenuClasses.join(",.")}`);
    return dropdown ? wo(`.${dropdownMenuClasses[0]}-toggle`, dropdown) : null;
  };
  const toggleTabHandler = (self, add) => {
    const action = add ? E$1 : r;
    action(self.element, ut, tabClickHandler);
  };
  const tabClickHandler = (e2) => {
    const self = getTabInstance(e2.target);
    if (!self)
      return;
    e2.preventDefault();
    self.show();
  };
  class Tab extends BaseComponent {
    static selector = tabSelector;
    static init = tabInitCallback;
    static getInstance = getTabInstance;
    /** @param target the target element */
    constructor(target) {
      super(target);
      const { element } = this;
      const content = getTargetElement(element);
      if (!content)
        return;
      const nav = de(element, ".nav");
      const container = de(content, ".tab-content");
      this.nav = nav;
      this.content = content;
      this.tabContent = container;
      this.dropdown = getParentDropdown(element);
      const { tab } = getActiveTab(this);
      if (nav && !tab) {
        const firstTab = wo(tabSelector, nav);
        const firstTabContent = firstTab && getTargetElement(firstTab);
        if (firstTabContent) {
          Ln(firstTab, activeClass);
          Ln(firstTabContent, showClass);
          Ln(firstTabContent, activeClass);
          F(element, Se, "true");
        }
      }
      toggleTabHandler(this, true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return tabComponent;
    }
    // TAB PUBLIC METHODS
    // ==================
    /** Shows the tab to the user. */
    show() {
      const { element, content: nextContent, nav, dropdown } = this;
      if (!(nav && Zn.get(nav)) && !In(element, activeClass)) {
        const { tab, content } = getActiveTab(this);
        if (nav)
          tabPrivate.set(nav, { tab, content, currentHeight: 0, nextHeight: 0 });
        hideTabEvent.relatedTarget = element;
        if (u(tab))
          Q(tab, hideTabEvent);
        if (hideTabEvent.defaultPrevented)
          return;
        Ln(element, activeClass);
        F(element, Se, "true");
        const activeDropdown = u(tab) && getParentDropdown(tab);
        if (activeDropdown && In(activeDropdown, activeClass)) {
          On(activeDropdown, activeClass);
        }
        if (nav) {
          const toggleTab = () => {
            if (tab) {
              On(tab, activeClass);
              F(tab, Se, "false");
            }
            if (dropdown && !In(dropdown, activeClass))
              Ln(dropdown, activeClass);
          };
          if (content && (In(content, fadeClass) || nextContent && In(nextContent, fadeClass))) {
            Zn.set(nav, toggleTab, 1);
          } else
            toggleTab();
        }
        if (content) {
          On(content, showClass);
          if (In(content, fadeClass)) {
            Un(content, () => triggerTabHide(this));
          } else {
            triggerTabHide(this);
          }
        }
      }
    }
    /** Removes the `Tab` component from the target element. */
    dispose() {
      toggleTabHandler(this);
      super.dispose();
    }
  }
  const toastString = "toast";
  const toastComponent = "Toast";
  const toastSelector = `.${toastString}`;
  const toastDismissSelector = `[${dataBsDismiss}="${toastString}"]`;
  const toastToggleSelector = `[${dataBsToggle}="${toastString}"]`;
  const showingClass = "showing";
  const hideClass = "hide";
  const toastDefaults = {
    animation: true,
    autohide: true,
    delay: 5e3
  };
  const getToastInstance = (element) => Bn(element, toastComponent);
  const toastInitCallback = (element) => new Toast(element);
  const showToastEvent = Jn(`show.bs.${toastString}`);
  const shownToastEvent = Jn(`shown.bs.${toastString}`);
  const hideToastEvent = Jn(`hide.bs.${toastString}`);
  const hiddenToastEvent = Jn(`hidden.bs.${toastString}`);
  const showToastComplete = (self) => {
    const { element, options } = self;
    On(element, showingClass);
    Zn.clear(element, showingClass);
    Q(element, shownToastEvent);
    if (options.autohide) {
      Zn.set(element, () => self.hide(), options.delay, toastString);
    }
  };
  const hideToastComplete = (self) => {
    const { element } = self;
    On(element, showingClass);
    On(element, showClass);
    Ln(element, hideClass);
    Zn.clear(element, toastString);
    Q(element, hiddenToastEvent);
  };
  const hideToast = (self) => {
    const { element, options } = self;
    Ln(element, showingClass);
    if (options.animation) {
      Xn(element);
      Un(element, () => hideToastComplete(self));
    } else {
      hideToastComplete(self);
    }
  };
  const showToast = (self) => {
    const { element, options } = self;
    Zn.set(
      element,
      () => {
        On(element, hideClass);
        Xn(element);
        Ln(element, showClass);
        Ln(element, showingClass);
        if (options.animation) {
          Un(element, () => showToastComplete(self));
        } else {
          showToastComplete(self);
        }
      },
      17,
      showingClass
    );
  };
  const toggleToastHandlers = (self, add) => {
    const action = add ? E$1 : r;
    const { element, triggers, dismiss, options } = self;
    if (dismiss) {
      action(dismiss, ut, self.hide);
    }
    if (options.autohide) {
      [tt, et, mt, gt].forEach(
        (e2) => action(element, e2, interactiveToastHandler)
      );
    }
    if (triggers.length) {
      triggers.forEach((btn) => action(btn, ut, toastClickHandler));
    }
  };
  const completeDisposeToast = (self) => {
    Zn.clear(self.element, toastString);
    toggleToastHandlers(self);
  };
  const toastClickHandler = (e2) => {
    const { target } = e2;
    const trigger = target && de(target, toastToggleSelector);
    const element = trigger && getTargetElement(trigger);
    const self = element && getToastInstance(element);
    if (!self)
      return;
    if (trigger && trigger.tagName === "A")
      e2.preventDefault();
    self.relatedTarget = trigger;
    self.show();
  };
  const interactiveToastHandler = (e2) => {
    const element = e2.target;
    const self = getToastInstance(element);
    const { type, relatedTarget } = e2;
    if (!self || element === relatedTarget || element.contains(relatedTarget))
      return;
    if ([mt, tt].includes(type)) {
      Zn.clear(element, toastString);
    } else {
      Zn.set(element, () => self.hide(), self.options.delay, toastString);
    }
  };
  class Toast extends BaseComponent {
    static selector = toastSelector;
    static init = toastInitCallback;
    static getInstance = getToastInstance;
    /**
     * @param target the target `.toast` element
     * @param config the instance options
     */
    constructor(target, config) {
      super(target, config);
      const { element, options } = this;
      if (options.animation && !In(element, fadeClass))
        Ln(element, fadeClass);
      else if (!options.animation && In(element, fadeClass))
        On(element, fadeClass);
      this.dismiss = wo(toastDismissSelector, element);
      this.triggers = [...Mo(toastToggleSelector, d(element))].filter(
        (btn) => getTargetElement(btn) === element
      );
      this.show = this.show.bind(this);
      this.hide = this.hide.bind(this);
      toggleToastHandlers(this, true);
    }
    /**
     * Returns component name string.
     */
    get name() {
      return toastComponent;
    }
    /**
     * Returns component default options.
     */
    get defaults() {
      return toastDefaults;
    }
    /**
     * Returns *true* when toast is visible.
     */
    get isShown() {
      return In(this.element, showClass);
    }
    // TOAST PUBLIC METHODS
    // ====================
    /** Shows the toast. */
    show() {
      const { element, isShown } = this;
      if (element && !isShown) {
        Q(element, showToastEvent);
        if (showToastEvent.defaultPrevented)
          return;
        showToast(this);
      }
    }
    /** Hides the toast. */
    hide() {
      const { element, isShown } = this;
      if (element && isShown) {
        Q(element, hideToastEvent);
        if (hideToastEvent.defaultPrevented)
          return;
        hideToast(this);
      }
    }
    /** Removes the `Toast` component from the target element. */
    dispose() {
      const { element, isShown } = this;
      if (isShown) {
        On(element, showClass);
      }
      completeDisposeToast(this);
      super.dispose();
    }
  }
  const componentsList = {
    Alert,
    Button,
    Carousel,
    Collapse,
    Dropdown,
    Modal,
    Offcanvas,
    Popover,
    Tab,
    Toast
  };
  const initComponentDataAPI = (callback, collection) => {
    [...collection].forEach((x2) => callback(x2));
  };
  const removeComponentDataAPI = (component, context) => {
    const compData = L.getAllFor(component);
    if (compData) {
      [...compData].forEach(([element, instance]) => {
        if (context.contains(element))
          instance.dispose();
      });
    }
  };
  const initCallback = (context) => {
    const lookUp = context && context.nodeName ? context : document;
    const elemCollection = [...pe("*", lookUp)];
    jn(componentsList).forEach((cs) => {
      const { init, selector } = cs;
      initComponentDataAPI(
        init,
        elemCollection.filter((item) => ko(item, selector))
      );
    });
  };
  const removeDataAPI = (context) => {
    const lookUp = context && context.nodeName ? context : document;
    Gn(componentsList).forEach((comp) => {
      removeComponentDataAPI(comp, lookUp);
    });
  };
  if (document.body)
    initCallback();
  else {
    E$1(document, "DOMContentLoaded", () => initCallback(), { once: true });
  }
  exports.Alert = Alert;
  exports.Button = Button;
  exports.Carousel = Carousel;
  exports.Collapse = Collapse;
  exports.Dropdown = Dropdown;
  exports.Listener = eventListener;
  exports.Modal = Modal;
  exports.Offcanvas = Offcanvas;
  exports.Popover = Popover;
  exports.Tab = Tab;
  exports.Toast = Toast;
  exports.initCallback = initCallback;
  exports.removeDataAPI = removeDataAPI;
  Object.defineProperty(exports, Symbol.toStringTag, { value: "Module" });
  return exports;
}({});
