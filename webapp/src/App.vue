<template>
  <v-app>
    <v-app-bar
      app
      color="primary"
      clipped-left
      dark
    >
      <v-app-bar-nav-icon @click.stop="drawer = !drawer"></v-app-bar-nav-icon>
      <v-spacer></v-spacer>
      <v-menu offset-y>
        <template v-slot:activator=" {on, attrs}">
          <v-btn color="secondary" dark v-bind="attrs" v-on="on">
            {{ activeDevice }}
          </v-btn>
        </template>
        <v-list>
          <v-list-item v-for="key in Object.keys($store.state.devices)" :key="key" @click="changeDevice(key)">
            <v-list-item-title v-text="key"></v-list-item-title>
          </v-list-item>
        </v-list>
      </v-menu>
      <v-spacer></v-spacer>
      <v-btn
        href="https://github.com/LibreSolar/data-manager-firmware"
        target="_blank"
        text
      >
        <span class="mr-2" v-if="$vuetify.breakpoint.mdAndUp">Latest Release</span>
        <v-icon>mdi-open-in-new</v-icon>
      </v-btn>
    </v-app-bar>

    <v-main class="accent" v-if="!$store.state.loading">
        <router-view/>
    </v-main>

    <v-navigation-drawer v-model="drawer" app clipped :expand-on-hover="$vuetify.breakpoint.lgAndUp">
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
import info from "../info.json"

export default {
  name: 'App',
  created() {
    this.$store.state.info = info
    this.$ajax
      .get('api/v1/ts/')
      .then(res => {
        if (res.data) {
          this.$store.state.devices = res.data
          this.$store.state.activeDevice = Object.keys(res.data)[0]
          this.$store.state.activeDeviceId = Object.values(res.data)[0]
          this.activeDevice = "Device: " + this.$store.state.activeDevice
          this.$store.state.loading = false
        }
      })
  },
  data () {
    return {
      drawer: null,
      items: [
        { title: 'Device Info', href: '/', icon: 'mdi-home' },
        { title: 'Live View', href: '/live', icon: 'mdi-chart-line' },
        { title: 'Configuration', href: '/config', icon: 'mdi-tune' },
        { title: 'Inout/Output', icon: 'mdi-swap-horizontal' },
        { title: 'Data Log', icon: 'mdi-chart-bar' },
        { title: 'Firmware Upgrade', href: '/ota', icon: 'mdi-upload'}
      ],
      right: null,
      activeDevice: "No Devices connected...",
      dialog: false
    }
  },
  methods: {
    changeDevice: function(key) {
      this.$store.state.activeDevice = key
      this.$store.state.activeDeviceId = this.$store.state.devices[key]
    }
  }
};
</script>