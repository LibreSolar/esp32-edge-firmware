<template>
 <v-container fill-height fluid>
    <v-layout text-center align-center>
      <v-flex>
        <v-card class="mx-auto my-auto" max-width="1200">
          <v-card-title primary-title class="justify-center">Configuration</v-card-title>
          <v-tabs v-model="tab" background-color=secodary grow>
            <v-tab
            v-for="section in dataObjects"
            :key="section.name">{{section.name}}</v-tab>
          </v-tabs>
          <v-tabs-items v-model="tab">
          <v-tab-item
            v-for="section in dataObjects"
            :key="section.name"
          >
            <v-card-text>
            <v-row justify="center" dense no-gutters>
                <v-col
                  v-for="option in section.options"
                  :key="option"
                  cols=12
                  md="6"
                  sm="8"
                  lg="6"
                  class="px-1 ma-1">
                  <v-text-field
                    :label="option.name"
                    v-model="option.value"
                    outlined
                    :dense="$vuetify.breakpoint.smAndDown"
                    class="pa-0 ma-0"
                  ></v-text-field>
                </v-col>
            </v-row>
          </v-card-text>
        <v-container class="text-center pa-md-0" v-if="section.options.length > 0">
            <v-btn color="primary" @click="sendValues" class="mx-4">
              <v-icon left>mdi-checkbox-marked-circle</v-icon> Save
            </v-btn>
            <v-btn @click="resetValues" class="mx-4">
              <v-icon left>mdi-cancel</v-icon> Cancel
            </v-btn>
        </v-container>
        </v-tab-item>
        </v-tabs-items>
          <v-card-text>
            <v-alert
              v-model="alert"
              dense
              text
              type="warning"
              transition="scale-transition">
            <v-row align="center">
              <v-col class="grow">{{ status }}</v-col>
              </v-row></v-alert>
          </v-card-text>
        </v-card>
      </v-flex>
    </v-layout>
  </v-container>
</template>

<script>
export default {
  data() {
    return {
      dataObjects:[
        {name: 'General', options: [
          {name: 'Hostname', value: 'esp32-edge'},
          {name: 'anything', value: 'something'}
        ]},
        {name: 'EmonCMS', options: []}
      ],
      baseData: null,
      diff: {},
      tab: null,
      dialog: false,
      status: "",
      alert: false,
    }
  }
};
</script>
