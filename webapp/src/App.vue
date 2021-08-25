<template>
  <v-app>
    <v-app-bar
      app
      color="primary"
      clipped-left
      dark
    >
      <v-app-bar-nav-icon v-if="sideBar" @click.stop="drawer = !drawer"></v-app-bar-nav-icon>
      <v-spacer></v-spacer>
      <v-menu offset-y>
        <template v-slot:activator=" {on, attrs}">
          <v-btn color="secondary" dark v-bind="attrs" v-on="on">
            {{ $store.state.activeDevice }}
          </v-btn>
        </template>
        <v-list>
          <v-list-item
            v-for="key in Object.keys($store.state.devices)"
            :key="key"
            @click="$store.commit('changeDevice', key); sideBar = true"
            link to="/info"
          >
            <v-list-item-title v-text="key"></v-list-item-title>
          </v-list-item>
        </v-list>
      </v-menu>
      <v-spacer></v-spacer>
      <v-btn icon large link to="/esp-config" class="primary" @click="sideBar = false">
        <v-icon>mdi-cog</v-icon>
      </v-btn>
    </v-app-bar>

    <v-main class="accent">
      <v-alert
      v-model="showAlert"
      dense
      :type="$store.state.globAlertType"
      transition="scale-transition"
      dismissible
      class="ma-4"
      >{{ $store.state.globAlertMsg }}</v-alert>
      <router-view/>
    </v-main>

    <v-navigation-drawer v-model="drawer" app clipped v-if="sideBar">
      <v-list dense nav>
        <v-list-item
          v-for="item in items"
          :key="item.title"
          link :to="item.href"
        >
          <v-list-item-icon>
            <v-icon>{{ item.icon }}</v-icon>
          </v-list-item-icon>

          <v-list-item-content>
            <v-list-item-title>{{ item.title }}</v-list-item-title>
          </v-list-item-content>
        </v-list-item>
      </v-list>
    </v-navigation-drawer>

    <v-footer color="primary">
      <v-spacer></v-spacer>
      <span class="white--text">Made with <v-icon small dark>mdi-heart</v-icon> by Libre Solar</span>
      <v-spacer></v-spacer>
    </v-footer>
  </v-app>
</template>

<script>
export default {
  name: 'App',
  created() {
    this.$store.dispatch('getDevices')
  },
  computed: {
    showAlert: {
      get () {
        return this.$store.state.globAlert
      },
      set () {
        this.$store.commit('resetAlert')
      }
    }
  },
  data () {
    return {
      drawer: null,
      items: [
        { title: 'Device Info', href: '/info', icon: 'mdi-home' },
        { title: 'Live View', href: '/live', icon: 'mdi-chart-line' },
        { title: 'Configuration', href: '/config', icon: 'mdi-tune' },
        { title: 'Input/Output', href: '/io', icon: 'mdi-swap-horizontal' },
        { title: 'Data Log', href: '/data-log', icon: 'mdi-chart-bar' },
        { title: 'Firmware Upgrade', href: '/ota', icon: 'mdi-upload'}
      ],
      sideBar: false
    }
  }
};
</script>
