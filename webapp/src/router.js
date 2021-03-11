import Vue from 'vue'
import Router from 'vue-router'
import Home from './views/Home.vue'
import Live from './views/Live.vue'
import Config from './views/Config.vue'
import IO from './views/IO.vue'
import Ota from './views/Ota.vue'
import DataLog from './views/DataLog.vue'


Vue.use(Router)

export default new Router({
  mode: 'history',
  base: process.env.BASE_URL,
  routes: [
    {
      path: '/',
      name: 'home',
      component: Home
    },
    {
      path: '/live',
      name: 'live',
      component: Live
    },
    {
      path: '/config',
      name: 'config',
      component: Config
    },
    {
      path: '/io',
      name: 'io',
      component: IO
    },
    {
      path: '/data-log',
      name: 'data-log',
      component: DataLog
    },
    {
      path: '/ota',
      name: 'ota',
      component: Ota
    }
  ]
})
