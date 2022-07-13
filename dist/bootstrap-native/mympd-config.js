import EventListener from '@thednp/event-listener';

import Alert from './components/alert-native';
import Button from './components/button-native';
import Carousel from './components/carousel-native';
import Collapse from './components/collapse-native';
import Dropdown from './components/dropdown-native';
import Modal from './components/modal-native';
import Offcanvas from './components/offcanvas-native';
import Popover from './components/popover-native';
import Tab from './components/tab-native';
import Toast from './components/toast-native';

import Version from './version';
import { initCallback, removeDataAPI } from './util/mympd-init.js';

const BSN = {
  Alert,
  Button,
  Carousel,
  Collapse,
  Dropdown,
  Modal,
  Offcanvas,
  Popover,
  Tab,
  Toast,

  initCallback,
  removeDataAPI,
  Version,
  EventListener,
};

export default BSN;
